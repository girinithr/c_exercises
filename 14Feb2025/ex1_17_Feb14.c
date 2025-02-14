/*Exercise 1-17. Write a program to print 
all input lines that are longer than 80 characters. */
#include <stdio.h>
#define MAXLINE 1000   /* maximum input line length */
int get_line(char line[], int maxline);
void copy(char to[], char from[]);
/* print the longest input line */
main()
{
   
    int len;            /* current line length */
    char line[MAXLINE];    /* current input line */
    char longest[MAXLINE]; /* longest line saved here */
    while ((len = get_line(line, MAXLINE)) > 0){
       if (len > 80) {
        //checks if the input is greater than 80 then prints it
           printf("%s",line);
        }
    }
}
   /* getline:  read a line into s, return length  */
int get_line(char s[],int lim)
{
   int c, i;
   for (i=0; i < lim-1 && (c=getchar())!=EOF && c!='\n'; ++i)
       s[i] = c;
   if (c == '\n') {
       s[i] = c;
       ++i;
   }
   s[i] = '\0';
   return i;
}
