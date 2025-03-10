/*Exercise 7-8. Write a program to print a set of files, starting each new one on a new page, with a title and a running page count for each file.*/

#include <stdio.h>  // Required for FILE, fopen, fclose, fgets, etc.
#include <stdlib.h> // Required for exit()
#include <string.h>

#define MAXFILES 100    // number of files supported from command line arguments
#define WIDTH 100       // width in chars for printing. Also maximum size a line can be when read. Actual chars read will be WIDTH - 5 due to line number printing
#define LINESPERPAGE 15 // number of lines printed before starting a new page

int my_getline(char *line, int max, FILE *fp);
void printPageNum(int num);

FILE *fp[MAXFILES] = { 0 }; // array of FILE pointers initialized to null
int names[MAXFILES];
int fpIndex = 0;

// print lines that match pattern. Source can be files or stdin
int main(int argc, char *argv[])
{
    char line[WIDTH];
    unsigned int lineNum = 1, pageNum = 1;
    for (int i = 1; i < argc; i++)
        if (fpIndex < MAXFILES)
        {
            if ((fp[fpIndex] = fopen(argv[i], "r")) == NULL)
            {
                fprintf(stderr, "can't open %s\n", argv[i]);
                exit(1);
            }
            names[fpIndex++] = i; // capture the index of the file name to use for printing later
        }
        else
        {
            fprintf(stderr, "too many input files, max supported is %d\n", MAXFILES);
            exit(1);
        }
    if (fpIndex == 0) // no input files
    {
        fprintf(stderr, "usage: print file1 [file2] [file3] [...]\n");
        exit(1);
    }
    for (int i = 0; i < MAXFILES && fp[i] != NULL; i++)
    {
        // prints the title
        int slen = strlen(argv[names[i]]);
        for (int j = 0; j < WIDTH; j++)
            putchar('=');
        putchar('\n');
        for (int j = 0; j < (WIDTH / 2) - (slen / 2); j++)
            putchar(' ');
        printf("%s", argv[names[i]]);
        putchar('\n');
        for (int j = 0; j < WIDTH; j++)
            putchar('=');
        putchar('\n');
        lineNum = 1;
        pageNum = 1;
        // prints the contents of the file
        while (my_getline(line, WIDTH - 4, fp[i]) != -1) // -4 for the 5 chars for printing line numbers and +1 for the '\0'
        {
            if ((lineNum - 1) % LINESPERPAGE == 0) // only print page number if more lines (if loop still running, there are more lines. If previous line needed page #, print it)
                printPageNum(pageNum++);
            printf("%03d: %s\n", lineNum++, line);

        }
        while((lineNum++ - 1) % LINESPERPAGE != 0) // start the next file on a new page
            putchar('\n');
    }
    return 0;
}

// read a line, return length. Removes \n if it ends in it
int my_getline(char *line, int max, FILE *fp)
{
    if (fgets(line, max, fp) == NULL)
        return -1;

    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    return strlen(line);
}

void printPageNum(int num)
{
    putchar('\n');
    for (int i = 0; i < (WIDTH / 2) - 4; i++) // page 001 == 8 chars (8 / 2 = 4)
            putchar('-');
    printf("Page %03d", num);
    for (int i = WIDTH / 2 + 4; i < WIDTH; i++)
            putchar('-');
    putchar('\n');
}
