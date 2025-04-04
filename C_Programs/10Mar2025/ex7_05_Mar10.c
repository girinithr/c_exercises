/*Exercise 7-5. Rewrite the postfix calculator of Chapter 4 to use scanf and/or sscanf to do
the input and number conversion.*/

#include <stdio.h>
#include <stdlib.h>

#define MAXOP 100 /* max size of operand or operator */

void push(double);
double pop(void);

int main()
{
    char *c;
    char s[MAXOP], buf[MAXOP];
    double a = 0, op2;
    char e = '\0';

    while (scanf("%s%c", s, &e) == 2) { /* get no-space string and space behind it */
        if (sscanf(s, " %lf", &a) == 1) /* is it a number */
            push(a);
        else if (sscanf(s, "%s", buf)) {
            for (c = buf ; *c; c++) {
                switch (*c) {
                    case '+':
                        push(pop() + pop());
                        break;
                    case '-':
                        op2 = pop();
                        push(pop() - op2);
                        break;
                    case '*':
                        push(pop() * pop());
                        break;
                    case '/':
                        op2 = pop();
                        if (op2 != 0.0)
                            push(pop() / op2);
                        else
                            printf("error: zero divisor\n");
                        break;
                    default:
                        printf("Unknown command\n");
                        break;
                    }
                } /* for */
            if (e == '\n') /* print result */
                printf("\t%.8g\n", pop());
            }
    }
    return 0;
}

#define MAXVAL 100  /* maximum depth of val stack */

static int sp = 0;  /* next free stack position */
static double val[MAXVAL]; /* value stack */

   /* push(): push f onto value stack */
void push(double f)
{
    if (sp < MAXVAL)
        val[sp++] = f;
    else
        printf("error: stack full, can't push %g\n", f);
}

   /* pop(): pop and return top value from stack */
double pop(void)
{
    if (sp > 0)
        return val[--sp];
    else {
        printf("error: stack empty\n");
        return 0.0;
    }
}
