/*Exercise 5-6. Rewrite appropriate programs from earlier chapters and exercises with pointers
instead of array indexing. Good possibilities include getline (Chapters 1 and 4), atoi, itoa,
and their variants (Chapters 2, 3, and 4), reverse (Chapter 3), and strindex and getop
(Chapter 4).*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define STR_MAX  10000
#define BUFSIZE  100
#define NUMBER   '0'

char *my_getline  (char *, int);
char *itoa 	  (int, char *);
int   atoi        (char *);
char *strrev      (char *);
int   strindex    (char *, char *);
int   getop       (char *);
int   getch       (void);
void  ungetch     (int);

int main(int argc, char *argv[])
{
        int num;
        char ss1[STR_MAX];
        char ss2[STR_MAX];
        char atoi_buf[STR_MAX];
        char itoa_buf[STR_MAX];
        int type;
        char s[BUFSIZE];

        printf (">>> Please enter a number: ");
        my_getline(atoi_buf, STR_MAX);
        num = atoi(atoi_buf);

        printf ("\natoi: str: %s, int: %d\n", atoi_buf, num);
        printf ("itoa: int: %d, str: %s\n\n", num, itoa(num, itoa_buf));

        printf (">>> Enter string 1: ");
        my_getline(ss1,STR_MAX);

        printf (">>> Enter string 2: ");
        my_getline(ss2,STR_MAX);

        printf ("\nThe reverse of string 1: %s\n", strrev(ss1));
        printf ("The reverse of string 2: %s\n", strrev(ss2));

        num = strindex(ss1, ss2);
        if (num == -1) {
                printf("\nThe substring (string 2) was not found in the ");
                printf("base string (string 1).\n\n");
        } else {
                printf("\nThe substring was found in the base string, ");
                printf("starting at position %d.\n\n", num);
        }

        printf (">>> Please enter a simple equation (parsing example using getop()).\n");
        printf (">>> Example: 100+1039.238-acd\n");

        while ((type = getop(s)) != EOF) {
                switch (type) {
                case NUMBER:
                        printf ("Found a number: %s\n", s);
                        break;
                case '+':
                        printf ("Found \'+\'\n");
                        break;
                case '\n':
                        printf ("Found Line Break.\n");
                        break;
                default:
                        printf ("Found something else: %s\n", s);
                        break;
                }
        }

        return 0;
}

char *my_getline(char *str, int str_max)
{
        char c;
        /*
         * A local variable to perform arithmetic on so that 'str' points to
         * the correct location when it is passed back to the caller.
         */
        char *s1 = str;
        while ((c = getchar()) != EOF && c != '\n' && str_max-- > 0 ) {
                *s1++ = c;
        }

        if (*s1 == '\n') {
                *s1++ = c;
        }

        *s1 = '\0';
        return str;
}

char *itoa(int num, char *str)
{
        char *ls = str;
        do
        {
                *ls++ = num % 10 + '0';
        } while ((num /= 10) > 0);
        strrev(str);
        return str;
}

int atoi(char *str)
{
        int n, sign;
        while (isspace(*str)) {
                str++;
        }
        sign = (*str == '-') ? -1 : 1;
        if (*str == '+' || *str == '-') {
                str++;
        }
        for (n = 0; isdigit(*str); str++) {
                n = 10 * n + (*str - '0');
        }
        return n * sign;
}

char *strrev(char *str)
{
        int i;
        char c;
        char *lp1 = str; /* The start of str. */
        char *lp2 = str; /* The end of str, for incrementing.  */
        char *lp3 = str; /* The end of str, for reference. */

        i =  strlen(str)-1;
        lp2 += i;
        lp3 += i;

        do
        {
                c = *lp1;
                *lp1++ = *lp2;
                *lp2-- = c;
        } while ((i -= 2) > 0);

        *++lp3 = '\0';
        return str;
}

int strindex(char *s, char *t)
{
        int i, j;
        char *sb = s;
        char *ss = s;
        char *tb = t;
        for (i = 0; *sb != '\0'; i++, sb++) {
                tb = t;  /* Reset the pointer to the beginning of the string. */
                ss = sb; /* Reset the substring pointer to the base string
                          * pointer. */
                for (j = 0; *tb != '\0' && *ss == *tb; ss++, tb++, j++) {
                        if (*(tb+1) == '\0' && j > 0) {
                                return i;
                        }
                }
        }
        return -1;
}

int getop (char *str)
{
        int c;
        while ((*str++ = c = getch()) == ' ' || c == '\t');

        *str = '\0';
        if (!isdigit(c) && c != '.') {
                return c;
        }

        if (isdigit(c)) {
                while (isdigit(*str++ = c = getch()));
        }

        if (c == '.') {         /* Collect fraction. */
                while (isdigit(*str++ = c = getch()));
        }

        *--str = '\0';          /* Compensate for the extra character. */
        if (c != EOF) {
                ungetch(c);     /* Return extra charater to the stack. */
        }

        return NUMBER;
}

char buf[BUFSIZE];              /* The Stack */
int  bufp = 0;                  /* Top Position on the stack */

int getch (void)
{
        return (bufp > 0) ? buf[--bufp] : getchar();
}

void ungetch (int c)
{
        if (bufp >= BUFSIZE) {
                printf("ungetch: too many characters.\n");
        } else {
                buf[bufp++] = c;
        }

}
