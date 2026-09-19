.PHONY: test disasm symbols

test:
	./bomb answers.txt

disasm:
	objdump -d -M intel bomb > bomb.asm

symbols:
	nm -n bomb
