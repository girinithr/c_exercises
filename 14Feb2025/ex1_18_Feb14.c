/*Exercise 1-18. Write a program to remove
trailing blanks and tabs from each line of input, and
 to delete entirely blank lines. */
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1000   /* maximum input line length */
int get_line(char line[], int maxline);
void copy(char to[], char from[]);

int main()
{

    int len;            /* current line length */
    char line[MAXLINE];    /* current input line */
    char longest[MAXLINE]; /* longest line saved here */
    while ((len = get_line(line, MAXLINE)) > 0){
       if (len > 0) {
        //checks if the input is greater than 80 then prints it
           printf("%s",line);
        }
    }
    return 0;
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
   while(s[i-1] == ' ' || s[i-1] == '\n'){
       s[i-1] = s[i];
       s[i] = NULL;
       --i;
   }
   s[i] = '\0';
   return i;
}
