/*Exercise 1-20. Write a program detab that replaces tabs in the input with the proper number
 of blanks to space to the next tab stop. Assume a fixed set of tab stops, say every n columns.
 Should n be a variable or a symbolic parameter? */
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1000   /* maximum input line length */
#define TABSTOP 5      /* tab stop interval */

int get_line(char s[], int lim);

int main() {
    int len;
    char line[MAXLINE];    /* current input line */

    while ((len = get_line(line, MAXLINE)) > 0) {
        printf("%s", line);  /* print the processed line */
    }

    return 0;
}

int get_line(char s[], int lim) {
    int c, i, pos;
    pos = 0;  /* start at column 0 */

    for (i = 0; i < lim - 1 && (c = getchar()) != EOF && c != '\n'; ++i) {
        if (c == '\t') {
            /* Calculate the number of spaces to insert */
            int next_stop = TABSTOP - (pos % TABSTOP);
            for (int j = 0; j < next_stop; ++j) {
                s[i + j] = ' ';  /* insert space */
            }
            i += next_stop - 1;  /* adjust index to reflect inserted spaces */
            pos += next_stop;  /* update the position */
        } else {
            s[i] = c;  /* copy the character */
            ++pos;     /* increment the position */
        }
    }

    if (c == '\n') {
        s[i] = c;  /* add newline at the end */
        ++i;
    }

    s[i] = '\0';  /* null-terminate the string */

    return i;  /* return the length of the string */
}
