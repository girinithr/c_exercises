/*Exercise 5-11. Modify the program entab and detab (written as exercises in Chapter 1) to
accept a list of tab stops as arguments. Use the default tab settings if there are no arguments.*/
#include <stdio.h>
#define MAX 1000
#define DEFAULT 8

main(int argc, char *argv[])
{
	int c, col, i, x;
	char s[MAX];
	int N;		/* tabstop value */
	int t_tabstops = 0;		/* total tabstops */

	/* The code inside ----- are the mofications */

	/* -----------------------------------------*/
	while(--argc > 0)
		t_tabstops++;
	/* -----------------------------------------*/

	i = 0;
	col = 1;
	while ((c = getchar()) != EOF) {
		if (c == '\t') {
			/* ------------------------------------- */
			if(t_tabstops){
				N = atoi(*++argv);
				--t_tabstops;
			}else
				N = DEFAULT;
			/* -------------------------------------- */

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
