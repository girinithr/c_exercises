/*Exercise 5-2. Write getfloat, the floating-point analog of getint. What type does getfloat
return as its function value?*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#define BUFFER 1000

void ungetch(int c);
int getch(void);
char getfloat(double *pn);

char buf[BUFFER];
int bufp=0;
int main()	{

	int n;
	double array[BUFFER];

	for (n = 0; n < BUFFER && getfloat(&array[n]) != EOF; n++);
	printf("%f\n", array[0]);

	return 0;
}
void ungetch(int c)	{
	if (bufp >= BUFFER)
		printf("ungetch: too many characters\n");
	else
		buf[bufp++] = c;
}

int getch(void)	{
	return (bufp > 0) ? buf[--bufp] : getchar();
}

char getfloat(double *pn)	{

	int c, sign;

	while (isspace(c = getch()));

	if (!isdigit(c) && c != EOF && c != '+' && c != '-')	{
		ungetch(c); //not a number
		return 0;
	}

	sign = (c == '-') ? -1 : 1;
	if (c == '+' || c == '-')
		c = getch();
	for (*pn = 0; isdigit(c); c = getch())
		*pn = 10 * *pn + (c - '0');
	if (c == '.')	{
		int i, dec;
		c = getch();
		for (i = 0; isdigit(c); c = getch(), i++)
			dec = 10 * dec + (c - '0');
		*pn += dec/(pow(10,i));
	}
	if (*pn == 0 && c != '0')	{ //if +/- is followed by NaN, pn will stay at 0
		ungetch(sign == 1 ? '+' : '-');
		return 0;
	}
	*pn *= sign;
	if (c != EOF)
		ungetch(c);
	return c;
}
