#include "csapp.h"
#include "cache.h"
#include <ctype.h>

#define HEADER_LIMIT 65536
#define HEADER_COUNT 128

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct {
    char host[MAXLINE], port[6], path[MAXLINE], authority[MAXLINE];
} target_t;

typedef struct {
    char *name;
    char *value;
} header_t;

static int valid_port(const char *s)
{
    unsigned long n = 0;
    if (!*s) return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        n = n * 10 + (*s++ - '0');
        if (n > 65535) return 0;
    }
    return n != 0;
}

static int parse_url(const char *url, target_t *t)
{
    char *host, *port = NULL, *end;
    size_t n;
    if (strncasecmp(url, "http://", 7)) return 0;
    url += 7;
    n = strcspn(url, "/?#");
    if (!n || n >= sizeof(t->authority)) return 0;
    memcpy(t->authority, url, n);
    t->authority[n] = '\0';
    strcpy(t->host, t->authority);
    host = t->host;
    if (*host == '[') {
        end = strchr(host, ']');
        if (!end || end == host + 1) return 0;
        if (end[1]) {
            if (end[1] != ':') return 0;
            port = end + 2;
        }
        *end = '\0';
        ++host;
    } else {
        port = strchr(host, ':');
        if (port) *port++ = '\0';
    }
    if (!*host || strpbrk(host, "@ \\\t\r\n")) return 0;
    if (port) {
        if (!valid_port(port)) return 0;
        snprintf(t->port, sizeof(t->port), "%lu", strtoul(port, NULL, 10));
    } else strcpy(t->port, "80");
    memmove(t->host, host, strlen(host) + 1);
    for (char *p = t->host; *p; ++p) *p = tolower((unsigned char)*p);
    url += n;
    n = strcspn(url, "#");
    if (n + 2 > sizeof(t->path)) return 0;
    t->path[0] = '\0';
    if (*url != '/') strcpy(t->path, "/");
    strncat(t->path, url, n);
    return 1;
}

static void error_reply(int fd, int code, const char *reason)
{
    char response[512];
    int n = snprintf(response, sizeof(response),
        "HTTP/1.0 %d %s\r\nConnection: close\r\n"
        "Content-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s\n",
        code, reason, strlen(reason) + 1, reason);
    rio_writen(fd, response, (size_t)n);
}

static int token_contains(const char *list, const char *word)
{
    while (*list) {
        size_t n;
        while (*list == ',' || isspace((unsigned char)*list)) ++list;
        n = strcspn(list, ",");
        size_t end = n;
        while (end && isspace((unsigned char)list[end - 1])) --end;
        if (end == strlen(word) && !strncasecmp(list, word, end)) return 1;
        list += n;
    }
    return 0;
}

static int hop_header(const char *name, header_t *h, int count)
{
    const char *fixed[] = {"Host", "User-Agent", "Connection", "Proxy-Connection",
        "Keep-Alive", "TE", "Trailer", "Transfer-Encoding", "Upgrade",
        "Proxy-Authorization", "Proxy-Authenticate", "Content-Length"};
    for (size_t i = 0; i < sizeof(fixed)/sizeof(*fixed); ++i)
        if (!strcasecmp(name, fixed[i])) return 1;
    for (int i = 0; i < count; ++i)
        if ((!strcasecmp(h[i].name, "Connection") ||
             !strcasecmp(h[i].name, "Proxy-Connection")) &&
            token_contains(h[i].value, name)) return 1;
    return 0;
}

/* 没有处理缓存有效期，带缓存控制头的直接跳过。 */
static int cacheable(const unsigned char *data, size_t size)
{
    char headers[HEADER_LIMIT], *end, *save, *line;
    size_t hsize = 0;
    long long length = -1;
    int status;
    for (size_t i = 3; i < size; ++i) {
        if (!memcmp(data + i - 3, "\r\n\r\n", 4)) {
            hsize = i + 1;
            break;
        }
    }
    if (!hsize || hsize >= sizeof(headers)) return 0;
    memcpy(headers, data, hsize);
    headers[hsize] = '\0';
    line = strtok_r(headers, "\r\n", &save);
    if (!line || sscanf(line, "HTTP/%*s %d", &status) != 1 || status != 200) return 0;
    while ((line = strtok_r(NULL, "\r\n", &save))) {
        char *colon = strchr(line, ':');
        if (!colon) return 0;
        *colon++ = '\0';
        while (isspace((unsigned char)*colon)) ++colon;
        if (!strcasecmp(line, "Content-Length")) {
            if (length >= 0 || !isdigit((unsigned char)*colon)) return 0;
            errno = 0;
            length = strtoll(colon, &end, 10);
            while (isspace((unsigned char)*end)) ++end;
            if (errno || *end || length < 0) return 0;
        }
        if (!strcasecmp(line, "Transfer-Encoding") || !strcasecmp(line, "Vary") ||
            !strcasecmp(line, "Set-Cookie") || !strcasecmp(line, "Cache-Control") ||
            !strcasecmp(line, "Pragma")) return 0;
    }
    return length < 0 || (unsigned long long)length == size - hsize;
}

static void serve(int client)
{
    rio_t reader;
    target_t target;
    char line[MAXLINE], method[16], url[MAXLINE], version[16], extra;
    char headers[HEADER_LIMIT], request[HEADER_LIMIT + 3 * MAXLINE];
    char key[3 * MAXLINE], *host = NULL;
    header_t fields[HEADER_COUNT];
    size_t used = 0, request_size, object_size = 0;
    int count = 0, use_cache = 1, server;
    ssize_t n;
    unsigned char object[MAX_OBJECT_SIZE], buffer[MAXBUF];
    rio_readinitb(&reader, client);
    n = rio_readlineb(&reader, line, sizeof(line));
    if (n <= 0) return;
    if (line[n - 1] != '\n' ||
        sscanf(line, "%15s %8191s %15s %c", method, url, version, &extra) != 3 ||
        (strcmp(version, "HTTP/1.0") && strcmp(version, "HTTP/1.1"))) {
        error_reply(client, 400, "Bad Request"); return;
    }
    if (strcmp(method, "GET")) {
        error_reply(client, 501, "Not Implemented");
        return;
    }
    if (!parse_url(url, &target)) {
        error_reply(client, 400, "Bad Request");
        return;
    }

    for (;;) {
        char *colon, *value, *name;
        n = rio_readlineb(&reader, line, sizeof(line));
        if (n <= 0) return;
        if (!strcmp(line, "\r\n") || !strcmp(line, "\n")) break;
        if (line[n - 1] != '\n' || used + (size_t)n + 1 > sizeof(headers) ||
            count == HEADER_COUNT) { error_reply(client, 431, "Headers Too Large"); return; }
        if (memchr(line, '\0', (size_t)n)) { error_reply(client, 400, "Bad Request"); return; }
        line[strcspn(line, "\r\n")] = '\0';
        name = headers + used;
        memcpy(name, line, strlen(line) + 1);
        used += strlen(line) + 1;
        colon = strchr(name, ':');
        if (!colon || colon == name) { error_reply(client, 400, "Bad Request"); return; }
        *colon = '\0';
        for (char *p = name; *p; ++p)
            if (!isalnum((unsigned char)*p) && !strchr("!#$%&'*+-.^_`|~", *p)) {
                error_reply(client, 400, "Bad Request"); return;
            }
        value = colon + 1;
        while (*value == ' ' || *value == '\t') ++value;
        for (char *p = value; *p; ++p)
            if ((unsigned char)*p < 32 && *p != '\t') {
                error_reply(client, 400, "Bad Request"); return;
            }
        fields[count++] = (header_t){name, value};
        if (!strcasecmp(name, "Host")) {
            if (host || !*value) { error_reply(client, 400, "Bad Request"); return; }
            host = value;
        }
        if (!strcasecmp(name, "Transfer-Encoding") ||
            (!strcasecmp(name, "Content-Length") && strcmp(value, "0"))) {
            error_reply(client, 400, "Unsupported Request Body"); return;
        }
        if (!strcasecmp(name, "Authorization") || !strcasecmp(name, "Cookie") ||
            !strcasecmp(name, "Range") || !strncasecmp(name, "If-", 3) ||
            !strcasecmp(name, "Cache-Control") || !strcasecmp(name, "Pragma")) use_cache = 0;
    }
    if (!host) host = target.authority;
    n = snprintf(key, sizeof(key), "%s:%s%s|%s", target.host, target.port, target.path, host);
    if (n < 0 || (size_t)n >= sizeof(key)) { error_reply(client, 414, "URI Too Long"); return; }
    if (use_cache && cache_get(key, object, &object_size)) {
        rio_writen(client, object, object_size);
        return;
    }
    request_size = (size_t)snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\nHost: %s\r\n%sConnection: close\r\nProxy-Connection: close\r\n",
        target.path, host, user_agent_hdr);
    for (int i = 0; i < count; ++i) {
        if (hop_header(fields[i].name, fields, count)) continue;
        n = snprintf(request + request_size, sizeof(request) - request_size,
                     "%s: %s\r\n", fields[i].name, fields[i].value);
        if (n < 0 || (size_t)n >= sizeof(request) - request_size) {
            error_reply(client, 431, "Headers Too Large"); return;
        }
        request_size += (size_t)n;
    }
    if (request_size + 2 >= sizeof(request)) { error_reply(client, 431, "Headers Too Large"); return; }
    memcpy(request + request_size, "\r\n", 2);
    request_size += 2;
    server = open_clientfd(target.host, target.port);
    if (server < 0) { error_reply(client, 502, "Bad Gateway"); return; }
    struct timeval timeout = {20, 0};
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(server, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (rio_writen(server, request, request_size) < 0) {
        close(server);
        error_reply(client, 502, "Bad Gateway");
        return;
    }
    while ((n = read(server, buffer, sizeof(buffer))) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (rio_writen(client, buffer, (size_t)n) < 0) {
            n = -1;
            break;
        }
        if (use_cache && object_size + (size_t)n <= sizeof(object)) {
            memcpy(object + object_size, buffer, (size_t)n);
            object_size += (size_t)n;
        } else {
            use_cache = 0;
        }
    }
    close(server);
    if (n == 0 && use_cache && cacheable(object, object_size))
        cache_put(key, object, object_size);
}

static void *worker(void *arg)
{
    int fd = *(int *)arg;
    free(arg);
    pthread_detach(pthread_self());
    struct timeval timeout = {20, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    serve(fd);
    close(fd);
    return NULL;
}

int main(int argc, char **argv)
{
    int listenfd;
    if (argc != 2 || !valid_port(argv[1])) {
        fprintf(stderr, "Usage: %s <port 1..65535>\n", argv[0]); return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    listenfd = open_listenfd(argv[1]);
    if (listenfd < 0) return 1;
    for (;;) {
        pthread_t tid;
        int fd = accept(listenfd, NULL, NULL);
        int *arg;
        if (fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        arg = malloc(sizeof(*arg));
        if (!arg) {
            close(fd);
            continue;
        }
        *arg = fd;
        if (pthread_create(&tid, NULL, worker, arg)) {
            free(arg);
            close(fd);
        }
    }
}
