/*Exercise 4-7. Write a routine ungets(s) that will push back an entire string onto the input.
Should ungets know about buf and bufp, or should it just use ungetch?*/
#include <stdio.h>
#include <string.h> // for strlen()


#define BUFSIZE 100 // buffer size for getch and ungetch

char buf[BUFSIZE];  // buffer for ungetch
int bufp = 0;       // next free position in buf

int getch(void);
void ungetch(int c);
void ungets(char s[]);

int main()
{
    int i;
    char s[] = "Hello World11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    ungets(s);
    for (i = 0; i < 11; i++)
        printf("%c", getch());
    return 0;
}

int getch(void)
{
    return (bufp > 0) ? buf[--bufp] : getchar();
}

void ungetch(int c)
{
    if (bufp >= BUFSIZE)
        printf("ungetch: too many characters\n");
    else
        buf[bufp++] = c;
}

// puts the reversed string on buffer so it comes out in the original order when getch is called
void ungets(char s[])
{
    int i = strlen(s);
    while (i >= 0)
        ungetch(s[i--]);
}
