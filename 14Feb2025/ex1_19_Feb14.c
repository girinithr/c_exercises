/*Exercise 1-19. Write a function reverse(s) that reverses the character string s. Use it to
 write a program that reverses its input a line at a time. */
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1000   /* maximum input line length */
int get_line(char line[], int maxline);
char* reverse(char str[], int length);

int main()
{

    int len;            /* current line length */
    char line[MAXLINE];    /* current input line */
    char longest[MAXLINE]; /* longest line saved here */
    while ((len = get_line(line, MAXLINE)) > 0){
       if (len > 0) {
           printf("%s",reverse(line,len));
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
//this function helps to reverse the charaters of input line
char* reverse(char str[],int length){
    char temp;
    for(int i=0;i<(length)/2;i++){
        temp = str[i];
        str[i] = str[length-1-i];
        str[length-1-i] = temp;
        }
        return str;
}
