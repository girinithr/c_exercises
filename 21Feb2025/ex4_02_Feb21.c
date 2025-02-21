/*Exercise 4-2. Extend atof to handle scientific notation of the form
123.45e-6 where a floating-point number may be followed by e or E and an optionally signed exponent.*/
#include <stdio.h>
#include <ctype.h>

double atof(char s[]);

int main(void)
{
	printf("%f\n", atof("123.45e-6"));

}

double atof(char s[])
{
	double val, power, base, p;
	int i, sign, exp;

	for (i = 0; isspace(s[i]); i++)
		;
	sign = (s[i] == '-') ? -1 : 1;
	if (s[i] == '-' || s[i] == '+')
		++i;
	for (val = 0.0; isdigit(s[i]); i++)
		val = 10.0 * val + (s[i] - '0');
	if (s[i] == '.')
		i++;
	for (power = 1.0; isdigit(s[i]); i++) {
		val = 10.0 * val + (s[i] - '0');
		power *= 10.0;
	}
	if (s[i] == 'e' || s[i] == 'E')
		i++;
	else
		return sign * val/power;
	base = (s[i] == '-') ? 0.1 : 10.0;
	if (s[i] == '+' || s[i] == '-')
		i++;
	for (exp = 0; isdigit(s[i]); i++)
		exp = 10 * exp + (s[i] - '0');
	for (p = 1; exp > 0; --exp)
		p = p * base;

	return (sign * (val/power)) * p;
}
