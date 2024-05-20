#include <stdio.h>
#include <stdint.h>
#include "thread.h"
// int main() {
//     fixed x = INT_TO_FIXED(3);  // 3을 고정소숫점으로 변환
//     fixed y = INT_TO_FIXED(4);  // 4를 고정소숫점으로 변환
//     int n = 2;

//     fixed result;

//     // x 및 y 추가
//     result = FIXED_ADD(x, y);
//     printf("x + y = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x에서 y 빼기
//     result = FIXED_SUBTRACT(x, y);
//     printf("x - y = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x와 n을 더합니다.
//     result = FIXED_ADD_INT(x, n);
//     printf("x + n = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x에서 n 빼기
//     result = FIXED_SUBTRACT_INT(x, n);
//     printf("x - n = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x에 y를 곱합니다.
//     result = FIXED_MULTIPLY(x, y);
//     printf("x * y = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x에 n을 곱합니다.
//     result = FIXED_MULTIPLY_INT(x, n);
//     printf("x * n = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x를 y로 나누기
//     result = FIXED_DIVIDE(x, y);
//     printf("x / y = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     // x를 n으로 나누기
//     result = FIXED_DIVIDE_INT(x, n);
//     printf("x / n = %d (고정소숫점), %d (정수)\n", result, FIXED_TO_INT(result));

//     return 0;
// }