/*
 * CS:APP Data Lab
 *
 * 自学版答案
 *
 * bits.c —— 实验解答源文件，也是需要提交的文件。
 *
 * 警告：不要包含 <stdio.h>，否则会干扰 dlc 检查器。调试时仍可直接
 * 使用 printf，虽然编译器可能会给出警告，但在本实验中可以忽略。
 */

#if 0
/*
 * 学生须知
 *
 * 第一步：仔细阅读以下说明。
 */

通过补全本文件中的各个函数来完成 Data Lab。

整数题编码规则：

  用一行或多行 C 代码替换各函数中原有的 return 语句，代码应采用
  以下形式：

  int Funct(arg1, arg2, ...) {
      /* 简要说明实现原理 */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  每个 Expr 只能使用：
  1. 0 至 255（0xFF）之间的整数常量，不能使用 0xffffffff 等大常量；
  2. 函数参数和局部变量，不能使用全局变量；
  3. 一元整数运算符 ! 和 ~；
  4. 二元整数运算符 &、^、|、+、<< 和 >>。

  部分题目会进一步限制可用运算符。每个表达式可以包含多个运算符，
  并不要求每行只能使用一个运算符。

  明确禁止：
  1. 使用 if、do、while、for、switch 等控制结构；
  2. 定义或使用宏；
  3. 在本文件中定义额外函数；
  4. 调用函数；
  5. 使用 &&、||、-、?: 等未获允许的运算符；
  6. 使用任何形式的类型转换；
  7. 使用 int 以外的数据类型，因此也不能使用数组、结构体或联合体。

  可以假设机器：
  1. 使用 32 位二进制补码表示整数；
  2. 对有符号整数执行算术右移；
  3. 位移量小于 0 或大于 31 时，其行为不可预测。


符合规则的编码示例：
  /*
   * pow2plus1 —— 返回 2^x + 1，其中 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* 利用左移计算 2 的幂 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 —— 返回 2^x + 4，其中 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* 利用左移计算 2 的幂 */
     int result = (1 << x);
     result += 4;
     return result;
  }

浮点题编码规则

浮点题的规则较宽松，可以使用循环和条件控制，也可以使用 int 和
unsigned、任意整数常量，以及针对这两种类型的算术、逻辑和比较运算。

明确禁止：
  1. 定义或使用宏；
  2. 在本文件中定义额外函数；
  3. 调用函数；
  4. 使用任何形式的类型转换；
  5. 使用 int 和 unsigned 以外的数据类型，包括数组、结构体和联合体；
  6. 使用浮点数据类型、浮点运算或浮点常量。


注意事项：
  1. 使用 dlc（Data Lab Checker）检查答案是否符合编码规则；
  2. 每个函数都有运算符数量上限，由 dlc 检查；赋值符号 = 不计数；
  3. 使用 btest 测试函数的正确性；
  4. 可以使用 BDD 检查器对函数进行形式化验证；
  5. 每个函数头部注释中的最大运算符数为最终标准；若实验文档与本文件
     不一致，以本文件为准。

/*
 * 第二步：按照上述编码规则补全以下函数。
 *
 * 为避免评分时出现意外：
 * 1. 使用 dlc 检查答案是否符合编码规则；
 * 2. 使用 BDD 检查器形式化验证答案是否正确。
 */


#endif
//1
/*
 * bitXor —— 只使用 ~ 和 & 计算 x^y
 *   示例：bitXor(4, 5) = 1
 *   允许的运算符：~ &
 *   最大运算符数：14
 *   难度：1
 */
int bitXor(int x, int y) {
  /* 保留只在 x 和 y 其中一个数中出现的位。 */
  return ~(~x & ~y) & ~(x & y);
}
/*
 * tmin —— 返回最小的二进制补码整数
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：4
 *   难度：1
 */
int tmin(void) {
  /* 最小补码整数只有符号位为 1。 */
  return 1 << 31;
}
//2
/*
 * isTmax —— 如果 x 是最大的二进制补码整数则返回 1，否则返回 0
 *   允许的运算符：! ~ & ^ | +
 *   最大运算符数：10
 *   难度：1
 */
int isTmax(int x) {
  /* 对 Tmax 而言，x + (x + 1) 等于 -1；同时排除 x 等于 -1。 */
  int next = x + 1;
  return !(~(x + next)) & !!next;
}
/*
 * allOddBits —— 如果字中所有奇数编号位均为 1，则返回 1
 *   位编号从最低有效位 0 到最高有效位 31
 *   示例：allOddBits(0xFFFFFFFD) = 0，allOddBits(0xAAAAAAAA) = 1
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：12
 *   难度：2
 */
int allOddBits(int x) {
  /* 使用合法的单字节常量构造 0xAAAAAAAA。 */
  int mask = 0xAA | (0xAA << 8);
  mask = mask | (mask << 16);
  return !((x & mask) ^ mask);
}
/*
 * negate —— 返回 -x
 *   示例：negate(1) = -1
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：5
 *   难度：2
 */
int negate(int x) {
  return ~x + 1;
}
//3
/*
 * isAsciiDigit —— 如果 0x30 <= x <= 0x39，则返回 1
 *   该范围对应字符 '0' 到 '9' 的 ASCII 编码
 *   示例：isAsciiDigit(0x35) = 1
 *         isAsciiDigit(0x3a) = 0
 *         isAsciiDigit(0x05) = 0
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：15
 *   难度：3
 */
int isAsciiDigit(int x) {
  /* x - 0x30 与 0x39 - x 必须同时为非负数。 */
  int aboveLower = x + (~0x30 + 1);
  int belowUpper = 0x39 + (~x + 1);
  return !(aboveLower >> 31) & !(belowUpper >> 31);
}
/*
 * conditional —— 实现与 x ? y : z 相同的效果
 *   示例：conditional(2, 4, 5) = 4
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：16
 *   难度：3
 */
int conditional(int x, int y, int z) {
  /* 将 x 的真假值扩展为全 0 或全 1 的掩码。 */
  int truth = !!x;
  int mask = ~truth + 1;
  return (mask & y) | (~mask & z);
}
/*
 * isLessOrEqual —— 如果 x <= y 则返回 1，否则返回 0
 *   示例：isLessOrEqual(4, 5) = 1
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：24
 *   难度：3
 */
int isLessOrEqual(int x, int y) {
  /* 异号时不会发生减法溢出；同号时检查 y - x 的符号。 */
  int xNegative = (x >> 31) & 1;
  int yNegative = (y >> 31) & 1;
  int signsDiffer = xNegative ^ yNegative;
  int difference = y + (~x + 1);
  int differenceNonnegative = !(difference >> 31);
  return (signsDiffer & xNegative) | ((!signsDiffer) & differenceNonnegative);
}
//4
/*
 * logicalNeg —— 不使用 ! 运算符实现逻辑非
 *   示例：logicalNeg(3) = 0，logicalNeg(0) = 1
 *   允许的运算符：~ & ^ | + << >>
 *   最大运算符数：12
 *   难度：4
 */
int logicalNeg(int x) {
  /* 对任意非零 x，x 和 -x 中至少有一个数的符号位为 1。 */
  return ((x | (~x + 1)) >> 31) + 1;
}
/* howManyBits —— 返回使用二进制补码表示 x 所需的最少位数
 *   示例：howManyBits(12) = 5
 *         howManyBits(298) = 10
 *         howManyBits(-5) = 4
 *         howManyBits(0) = 1
 *         howManyBits(-1) = 1
 *         howManyBits(0x80000000) = 32
 *   允许的运算符：! ~ & ^ | + << >>
 *   最大运算符数：90
 *   难度：4
 */
int howManyBits(int x) {
  /* 先将负数规格化，再使用二分查找定位最高的 1 位。 */
  int b16, b8, b4, b2, b1;
  x = x ^ (x >> 31);
  b16 = !!(x >> 16) << 4;
  x = x >> b16;
  b8 = !!(x >> 8) << 3;
  x = x >> b8;
  b4 = !!(x >> 4) << 2;
  x = x >> b4;
  b2 = !!(x >> 2) << 1;
  x = x >> b2;
  b1 = !!(x >> 1);
  x = x >> b1;
  return b16 + b8 + b4 + b2 + b1 + x + 1;
}
//float
/*
 * floatScale2 —— 返回浮点数表达式 2*f 的位级等价值
 *   参数和返回值均以 unsigned 传递，但应解释为单精度浮点数的位表示。
 *   当参数为 NaN 时，直接返回原参数。
 *   允许的运算符：任意 int/unsigned 运算，包括 ||、&&、if 和 while
 *   最大运算符数：30
 *   难度：4
 */
unsigned floatScale2(unsigned uf) {
  unsigned sign = uf & 0x80000000u;
  unsigned exponent = uf & 0x7F800000u;

  if (exponent == 0x7F800000u) {
    return uf;
  }
  if (exponent == 0) {
    return sign | ((uf & 0x7FFFFFFFu) << 1);
  }
  exponent = exponent + 0x00800000u;
  if (exponent == 0x7F800000u) {
    return sign | exponent;
  }
  return sign | exponent | (uf & 0x007FFFFFu);
}
/*
 * floatFloat2Int —— 返回浮点表达式 (int)f 的位级等价值
 *   参数以 unsigned 传递，但应解释为单精度浮点数的位表示。
 *   对任何超出范围的值（包括 NaN 和无穷）返回 0x80000000u。
 *   允许的运算符：任意 int/unsigned 运算，包括 ||、&&、if 和 while
 *   最大运算符数：30
 *   难度：4
 */
int floatFloat2Int(unsigned uf) {
  unsigned sign = uf >> 31;
  unsigned exponent = (uf >> 23) & 0xFF;
  unsigned fraction = (uf & 0x007FFFFF) | 0x00800000;
  int power = exponent + (~127 + 1);
  unsigned value;

  if (exponent == 0xFF || power > 30) {
    return 0x80000000u;
  }
  if (power < 0) {
    return 0;
  }
  if (power > 23) {
    value = fraction << (power - 23);
  } else {
    value = fraction >> (23 - power);
  }
  if (sign) {
    return ~value + 1;
  }
  return value;
}
/*
 * floatPower2 —— 对任意 32 位整数 x，返回表达式 2.0^x 的位级等价值
 *
 *   返回的 unsigned 应与单精度浮点数 2.0^x 具有完全相同的位表示。
 *   如果结果小到无法表示为非规格化数，则返回 0；如果结果过大，
 *   则返回正无穷。
 *
 *   允许的运算符：任意 int/unsigned 运算，包括 ||、&&、if 和 while
 *   最大运算符数：30
 *   难度：4
 */
unsigned floatPower2(int x) {
  if (x < -149) {
    return 0;
  }
  if (x < -126) {
    return 1u << (x + 149);
  }
  if (x <= 127) {
    return (x + 127) << 23;
  }
  return 0x7F800000u;
}
