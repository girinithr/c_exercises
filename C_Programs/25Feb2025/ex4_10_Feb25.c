/*Exercise 4-10. An alternate organization uses getline to read an entire input line; this makes
getch and ungetch unnecessary. Revise the calculator to use this approach.*/
#include <stdio.h>
#include <stdlib.h>		/* for atof() */

#define MAXOP	100		/* max size of operand or operator */
#define NUMBER	'0'		/* signal that a number was found */
#define MAXLINE 1000	/* maximum input line */

char line[MAXLINE];
int index;

int getop(char []);
void push(double);
double pop(void);
int my_getline(char line[], int max);

/* reverse Polish Calculator */
main()
{
	int type;
	double op2;
	char s[MAXOP];
	int len;

	while((len = my_getline(line, MAXLINE)) > 0) {
		index = 0;				/* always start at the beginning of
								   the line */
		while(index < len) {
			type = getop(s);
			switch(type) {
			case NUMBER:
				push(atof(s));
				break;
			case '+':
				push(pop() + pop());
				break;
			case '*':
				push(pop() * pop());
				break;
			case '-':
				op2 = pop();
				push(pop() - op2);
				break;
			case '/':
				op2 = pop();
				if(op2 != 0.0)
					push(pop() / op2);
				else
					printf("error: zero divisor\n");
				break;
			case '\n':
				printf("\t%.8g\n", pop());
				break;
			default:
				printf("error: unknown command %s\n", s);
				break;
			}
		}
	}
	return 0;
}

/* getline: get line into s, return length */
int my_getline(char s[], int lim)
{
	int c, i;

	i = 0;
	while(--lim > 0 && (c=getchar()) != EOF && c != '\n')
		s[i++] = c;
	if(c == '\n')
		s[i++] = c;
	s[i] = '\0';
	return i;
}

#define MAXVAL	100		/* maximum depth of value stack */

int sp = 0;				/* next free stack position */
double val[MAXVAL];		/* value stack */

/* push: push f onto value stack */
void push(double f)
{
	if (sp < MAXVAL)
		val[sp++] = f;
	else
		printf("error: stack full, cant push %g\n", f);
}
/* pop: pop and return top value from stack */
double pop(void)
{
	if (sp > 0)
		return val[--sp];
	else {
		printf("error: stack empty\n");
		return 0.0;
	}
}

#include <ctype.h>

/* getop: get next operator or numeric operand */
int getop(char s[])
{
	int i, c;

	while((s[0] = c = line[index++]) == ' ' || c == '\t')
		;
	s[1] = '\0';
	if(!isdigit(c) && c != '.')
		return c;			/* not a number */
	i = 0;
	if (isdigit(c))			/* collect integer part */
		while (isdigit(s[++i] = c = line[index++]))
			;
	if (c == '.')			/* collect fraction part */
		while (isdigit(s[++i] = c = line[index++]))
			;
	s[i] = '\0';
	return NUMBER;
}
