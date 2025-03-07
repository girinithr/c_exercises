/*Exercise 5-17. Add a field-searching capability, so sorting may bee done on fields within lines,
each field sorted according to an independent set of options. (The index for this book was
sorted with -df for the index category and -n for the page numbers.)*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#define MAXLINES 5000

char *lineptr[MAXLINES];

int readlines(char *lineptr[], int nlines);
void writelines(char *lineptr[], int nlines, int reverse);

void q_sort(void *lineptr[], int left, int right,
        int (*comp)(void *, void *));
int numcmp(char *, char *);
int strfcmp(char *, char*);
int strdcmp(char *, char *);
int strdfcmp(char *, char *);

int pos1 = 0;
int pos2 = 0;

main(int argc, char *argv[]) {

    int nlines;
    int numeric = 0;
    int reverse = 0;
    int fold = 0;
    int dir = 0;

    while (--argc > 0 && (*++argv)[0] == '-') {
        int c;
        while (c = *++argv[0])
            switch (c) {
                case 'n':
                    numeric = 1;
                    break;
                case 'r':
                    reverse = 1;
                    break;
                case 'f':
                    fold = 1;
                    break;
                case 'd':
                    dir = 1;
                    break;
                default:
                    printf("Illegal option: %c\n", c);
                    break;
            }
    }
    if (argc > 0 && isdigit(*argv[0]) ) {
        pos1 = atoi(*argv);
        --argc;
        ++argv;
    }
    if (argc > 0 && isdigit(*argv[0]) ) {
        pos2 = atoi(*argv);
        --argc;
        ++argv;
    }

    if (argc != 0) {
        printf("Usage: sort -n -r -f [begin] [end]\n");
        return 1;
    }
    if (pos1 > pos2) {
        printf("Usage: sort -n -r -f [begin] [end]\n");
        printf("begin must not be greater than end\n");
        return 1;
    }

    printf(numeric ? "Numerical sort\n" : "Lexicographic sort\n");
    printf(reverse ? "Reverse sort\n" : "Normal sort\n");
    printf(fold ? "Case insensitive\n" : "Case sensitive\n");
    printf(dir ? "Directory order\n" : "");

    if ((nlines = readlines(lineptr, MAXLINES)) > 0) {
        if (numeric)
            q_sort((void **) lineptr, 0, nlines - 1,
                    (int (*)(void *, void *)) numcmp);
        else if (dir)
            if (fold)
                q_sort((void **) lineptr, 0, nlines - 1,
                        (int (*)(void *, void *)) strdfcmp);
            else
                q_sort((void **) lineptr, 0, nlines - 1,
                        (int (*)(void *, void *)) strdcmp);
        else
            if (fold)
                q_sort((void **) lineptr, 0, nlines - 1,
                        (int (*)(void *, void *)) strfcmp);
            else
                q_sort((void **) lineptr, 0, nlines - 1,
                        (int (*)(void *, void *)) strcmp);
        writelines(lineptr, nlines, reverse);
        return 0;
    } else {
        printf("input too big to sort\n");
        return 1;
    }
}

void q_sort(void *v[], int left, int right,
        int (*comp)(void *, void *)) {

    int i, last;
    void swap(void *v[], int, int);

    if (left >= right)
        return;
    swap(v, left, (left + right) / 2);
    last = left;
    for (i = left + 1; i <= right; i++)
        if ((*comp)(v[i], v[left]) < 0)
            swap(v, ++last, i);
    swap(v, left, last);
    q_sort(v, left, last - 1, comp);
    q_sort(v, last + 1, right, comp);
}

void substring(char *s, char *t) {

    int i,j,len;

    len = strlen(s);
    if (pos2 > 0 && pos2 < len)
        len = pos2;
    if (pos2 > 0 && len < pos2)
        printf("error: string %s is too short\n", s);
    for (j=0, i=pos1; i<len; i++, j++)
        t[j] = s[i];
    t[j] = '\0';

}


int strdcmp(char *s1, char *s2) {

    int i, j, endpos;
    char a, b;

    i = j = pos1;
    if (pos2 > 0)
        endpos = pos2;
    else if ( (endpos = strlen(s1)) > strlen(s2) )
        endpos = strlen(s2);

    do {
        while (i < endpos && !isalnum(*s1) && *s1 != ' ' && *s1 != '\0')
            s1++;
        while (j < endpos && !isalnum(*s2) && *s2 != ' ' && *s2 != '\0')
            s2++;

        if (i < endpos && j < endpos) {
            a = *s1;
            s1++;
            b = *s2;
            s2++;

            if (a == b && a == '\0')
               return 0;
        }
    } while (a == b && i < endpos && j < endpos);

    return a - b;

}

int strdfcmp(char *s1, char *s2) {

    int i, j, endpos;
    char a, b;

    i = j = pos1;
    if (pos2 > 0)
        endpos = pos2;
    else if ( (endpos = strlen(s1)) > strlen(s2) )
        endpos = strlen(s2);

    do {
        while (i < endpos && !isalnum(*s1) && *s1 != ' ' && *s1 != '\0')
            s1++;
        while (j < endpos && !isalnum(*s2) && *s2 != ' ' && *s2 != '\0')
            s2++;

        if (i < endpos && j < endpos) {
            a = tolower(*s1);
            s1++;
            b = tolower(*s2);
            s2++;

            if (a == b && a == '\0')
                return 0;
        }

    } while (a == b && i < endpos && j < endpos);

    return a - b;
}

int strfcmp(char *s1, char *s2) {

    int i, j, endpos;

    i = j = pos1;
    if (pos2 > 0)
        endpos = pos2;
    else if ( (endpos = strlen(s1)) > strlen(s2) )
        endpos = strlen(s2);

    for ( ; tolower(s1[i]) == tolower(s2[j]) && i < endpos && j < endpos; i++, j++)
        if (s1[i] == '\0')
            return 0;
    return tolower(s1[i])-tolower(s2[j]);
}

#define MAXSTR 100

int numcmp(char *s1, char *s2) {

    double v1, v2;
    char str[MAXSTR];

    substring(s1, str);
    v1 = atof(str);
    substring(s2, str);
    v2 = atof(str);

    if (v1 < v2)
        return -1;
    else if (v1 > v2)
        return 1;
    else
        return 0;
}

void swap(void *v[], int i, int j) {

    void *temp;

    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}


#define MAXLEN 1000

int get_line(char line[], int maxline);
char *alloc(int);

int readlines(char *lineptr[], int maxlines) {

    int len, nlines;
    char *p, line[MAXLEN];

    nlines = 0;
    while ((len = get_line(line, MAXLEN)) > 0)
        if (nlines >= MAXLINES || (p = alloc(len)) == NULL)
            return -1;
        else {
            line[len - 1] = '\0';
            strcpy(p, line);
            lineptr[nlines++] = p;
        }

    return nlines;

}

void writelines(char * lineptr[], int nlines, int reverse) {

    int i;

    if (reverse)
        for (i = nlines-1; i >= 0; i--)
            printf("%s\n", lineptr[i]);
    else
        for (i = 0; i < nlines; i++)
            printf("%s\n", lineptr[i]);
}

int get_line(char s[], int lim) {
    int c, i;

    for (i = 0; i < lim - 1 && (c = getchar()) != EOF && c != '\n'; ++i)
        s[i] = c;
    if (c == '\n') s[i++] = c;
    s[i] = '\0';
    return i;
}



#define ALLOCSIZE 10000

static char allocbuf[ALLOCSIZE];
static char *allocp = allocbuf;

char *alloc(int n) {
    if (allocbuf + ALLOCSIZE - allocp >= n) {
        allocp += n;
        return allocp - n;
    } else
        return 0;
}

void afree(char *p) {
    if (p >= allocbuf && p < allocbuf + ALLOCSIZE)
        allocp = p;
}
