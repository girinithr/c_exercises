/*Exercise 5-12. Extend entab and detab to accept the shorthand
entab -m +n
to mean tab stops every n columns, starting at column m. Choose convenient (for the user)
default behavior.*/
#include <stdio.h>
#define MAX 1000
#define DEFAULT 8		/* Default tabstop */

main(int argc, char *argv[])
{
	int c, col, i, x;
	char s[MAX];
	int N;
	int m = 0, n = DEFAULT;
	int k;

	while(--argc > 0){
		c = (*++argv)[0];
		switch(c){
			case '-':
				m = atoi(++argv[0]);
				break;
			case '+':
				n = atoi(++argv[0]);
				break;
		}
	}

	i = 0;
	col = 1;
	while ((c = getchar()) != EOF) {
		if (c == '\t') {
			if(col <= m + n)
				N = m + n;
			else{
				for(k = 1; col > m + n + (k * n); ++k)
					;
				N = m + n + (k * n);
			}
			x = ((col - 1) / N) + 1;
			while (col <= x * N) {
				s[i] = '*';		/* The character "*" is
								 * used to see the effect
								 * since a blank line will
								 * not be seen on the
								 * display */
				++i;
				/* Overflow  check */
				if (i == MAX - 1) {
					s[i] = '\0';
					printf("%s", s);
					i = 0;
				}
				++col;
			}
		} else {
			s[i] = c;
			++i;
			++col;
		}

		if (c == '\n')
			col = 1;

		/* Process the input when the array is full */
		if (i == MAX - 1) {
			s[i] = '\0';
			printf("%s", s);
			i = 0;
		}
	}
	/* Process the remaining input */
	s[i] = '\0';
	printf("%s", s);

	return 0;
}
