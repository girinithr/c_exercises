/*Exercise 3-2. Write a function escape(s,t) that converts characters like newline and tab into
visible escape sequences like \n and \t as it copies the string t to s. Use a switch. Write a
function for the other direction as well, converting escape sequences into the real characters.*/
#include <stdio.h>


int main()
{
    int c, i = 0;
    char s[10000];
    while ((c = getchar()) != EOF)
    {
        switch (c)
        {
        case '\n':
            printf("\\n");
            break;
        case '\t':
            printf("\\t");
            break;
        case '\b':
            printf("\\b");
            break;
        case '\v':
            printf("\\v");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\f':
            printf("\\f");
            break;
        case '\a':
            printf("\\a");
            break;
        default:
            putchar(c);
            break;
        }
        s[i] = c;
        i++;
    }
    printf("\n%s\n", s);
    return 0;
}
