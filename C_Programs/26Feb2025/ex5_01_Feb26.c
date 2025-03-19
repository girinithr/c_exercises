/*Exercise 5-1. As written, getint treats a + or - not followed by a digit as a valid
representation of zero. Fix it to push such a character back on the input.*/
#include <stdio.h>
#include <ctype.h> /* for isdigit(c), etc. */

#define MAX 10

int getint(int *pn);

int main(void)
{
	int i, num[MAX];
	int val;

	for (i = 0; i < MAX && (val = getint(&num[i])) != EOF; i++)
		printf("num[%d] = %d, \tvalue returned: %d (%s)\n", i, num[i],
				val, val != 0 ? "number" : "not a number");

	return 0;
}

int getch(void);
void ungetch(int c);

int getint(int *pn)
{
	int c, sign;

	*pn = 0;
	while (isspace(c = getch()))
		;
	if (!isdigit(c) && c != EOF && c != '+' && c != '-') {
		/* ungetch(c);*/ /* if returned to input we fill the array */
		return 0;
	}
	sign = (c == '-') ? -1 : 1;
	if (c == '+' || c == '-') {
		c = getch();
		if (!isdigit(c)) {
			ungetch(sign == 1 ? '+' : '-');
			return 0;
		}
	}
	while (isdigit(c)) {
		*pn = 10 * *pn + (c - '0');
		c = getch();
	}
	*pn *= sign;
	if (c != EOF)
		ungetch(c);

	return c;
}

int bufp = 0;
int buf[MAX];

int getch(void)
{
	return bufp > 0 ? buf[--bufp] : getchar();
}

void ungetch(int c)
{
	if (bufp < MAX)
		buf[bufp++] = c;
}
