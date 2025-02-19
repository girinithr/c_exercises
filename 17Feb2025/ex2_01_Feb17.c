/* Exercise 2-1. Write a program to determine the ranges of char, short, int, and long
 variables, both signed and unsigned, by printing appropriate values from standard headers
 and by direct computation. Harder if you compute them: determine the ranges of the various
 floating-point types. */
#include <stdio.h>
#include <limits.h>
float getFloat(char sign, unsigned char exp, unsigned mantissa);
double getDouble(char sign, unsigned short exp, unsigned long long mantissa);
int main(void)
{
  printf("Maximum numeric value of type char: %d\n", CHAR_MAX);
  printf("Minimum numeric value of type char: %d\n\n", CHAR_MIN);
  printf("Maximum value of type signed char: %d\n", SCHAR_MAX);
  printf("Minimum value of type signed char: %d\n\n", SCHAR_MIN);
  printf("Maximum value of type unsigned char: %u\n\n", (unsigned) UCHAR_MAX);
  printf("Maximum value of type short: %d\n", SHRT_MAX);
  printf("Minimum value of type short: %d\n\n", SHRT_MIN);
  printf("Maximum value of type unsigned short: %u\n\n", (unsigned) USHRT_MAX);
  printf("Maximum value of type int: %d\n", INT_MAX);
  printf("Minimum value of type int: %d\n\n", INT_MIN);
  printf("Maximum value of type unsigned int: %u\n\n", UINT_MAX);
  printf("Maximum value of type long: %ld\n", LONG_MAX);
  printf("Minimum value of type long: %ld\n\n", LONG_MIN);
  printf("Maximum value of type unsigned long: %lu\n\n", ULONG_MAX);
  //------------------------------------------------------------------------------------------
  printf("Maximum numeric value of type unsigned char: %d\n", (unsigned char)~0);
  printf("Minimum numeric value of type unsigned char: %d\n\n", (unsigned char)0);
  printf("Maximum value of type signed char: %d\n", (unsigned char)~0 >> 1);
  printf("Minimum value of type signed char: %d\n\n", ~(unsigned char)~0 >> 1);
  printf("Maximum value of type short: %d\n", (unsigned short)~0 >> 1);
  printf("Minimum value of type short: %d\n\n", ~((unsigned short)~0 >> 1));
  printf("Maximum value of type unsigned short: %u\n\n", (unsigned short)~0);
  printf("Maximum value of type int: %d\n", ~0U >> 1);
  printf("Minimum value of type int: %d\n\n", ~(~0U >> 1));
  printf("Maximum value of type unsigned int: %u\n\n", ~0UL);
  printf("Maximum value of type long: %ld\n", ~0UL >> 1);
  printf("Minimum value of type long: %ld\n\n", ~(~0UL >> 1));
  printf("Maximum value of type unsigned long: %lu\n\n", 0ULL);
  printf("Float[%g to %g]\n", getFloat(1, 0, 1), getFloat(0, ~0-1, ~0));
    printf("Double[%g to %g]\n", getDouble(1, 0, 1), getDouble(0, ~0-1, ~0));
  return 0;
}
float getFloat(char sign, unsigned char exp, unsigned mantissa)
{
    unsigned f = (unsigned)(sign != 0) << 31 | (unsigned)exp << 23 | mantissa & 0x7FFFFF;
    return *((float *)&f);
}
double getDouble(char sign, unsigned short exp, unsigned long long mantissa)
{
    unsigned long long d = (unsigned long long)(sign != 0) << 63 | (unsigned long long)(exp & 0x7FF) << 52 | mantissa & 0xFFFFFFFFFFFFF;
    return *((double *)&d);
}
