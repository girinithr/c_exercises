/*Exercise 5-7. Rewrite readlines to store lines in an array supplied by main, rather than
calling alloc to maintain storage. How much faster is the program?*/
#include <stdio.h>
#include <string.h>
#define MAXLINES 5000
#define MAXLEN 1000
#define MAXSTORE 10000 /* max space allocated for all lines.  Same as ALLOCSIZE on p.91. */


char *lineptr[MAXLINES];

int readlines(char *lineptr[], int nlines);
void writelines(char *lineptr[], int nlines);

void qsort(char *lineptr[], int left, int right);

main()
{
    int nlines;
    char linestore[MAXSTORE]; /* array for storing all lines */
    /* myreadlines will pass an extra parameter linestore for storing all the input lines */
    if ((nlines = myreadlines(lineptr, MAXLINES, linestore)) >= 0)
    {
        qsort(lineptr, 0, nlines-1);
        writelines(lineptr, nlines);
        return 0;
    }
    else
    {
        printf("error: input too big to sort\n");
        return 1;
    }
}

int my_getline(char *, int);

/* My solution to the problem, without using alloc */

int myreadlines(char *lineptr[], int maxlines, char *ls)
{
    int len, nlines;
    char *p, line[MAXLEN];
    nlines = 0;
    p = ls + strlen(ls); /* initiate to first empty position */
    while ((len = my_getline(line, MAXLEN)) > 0)
        /* The line below will check to make sure adding the nextline will not exceed MAXSTORE */
        if (nlines >= maxlines || (strlen(ls) + len) > MAXSTORE)
            return -1;
        else
        {   line[len-1] = '\0';
            strcpy(p, line);
            lineptr[nlines++] = p;
            p += len; /* point p to next empty position */
        }
    return nlines;
}


void writelines(char *lineptr[], int nlines)
{
    while (nlines-- > 0)
        printf("%s\n", *lineptr++);
}


void qsort(char *v[], int left, int right)
{
    int i, last;
    void swap(char *v[], int i, int j);
    if (left >= right)
        return;
    swap(v, left, (left + right)/2);
    last = left;
    for (i = left+1; i <= right; i++)
        if (strcmp(v[i], v[left]) < 0)
            swap(v, ++last, i);
    swap(v, left, last);
    qsort(v, left, last-1);
    qsort(v, last+1, right);
}


void swap(char *v[], int i, int j)
{
    char *temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}

int my_getline(char s[], int lim)
{
  int c, i;

  for (i = 0; i < lim - 1 && (c = getchar()) != EOF && c != '\n'; i++)
    s[i] = c;
  if (c == '\n') {
    s[i++] = c;
  }
  s[i] = '\0';
  return i;
}
