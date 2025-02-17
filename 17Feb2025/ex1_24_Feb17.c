/*Exercise 1-24. Write a program to check a C program for rudimentary syntax errors like
 unmatched parentheses, brackets and braces. Don't forget about quotes, both single and
 double, escape sequences, and comments.
*/
#include <stdio.h>

void quote(int c);
void comment(int c, int nc);

int main(void)
{

  int c,nc;
  int cbrace,sbrace,paran;
  cbrace = sbrace = paran = 0;

  while ((c = getchar()) != EOF)
  {
    if (c == '\'' || c == '"') quote(c);
    else if (c == '/'){
        nc  = getchar();
        if (nc == '*' || nc == '/')
            comment(c,nc);
    }
    else{
        if (c == '{') ++cbrace;
        else if (c == '}') --cbrace;
        else if( c == '[') ++sbrace;
        else if (c == ']') --sbrace;
        else if (c == '(') ++paran;
        else if ( c == ')') --paran;
    }
  }
  if(cbrace != 0 || sbrace != 0 || paran != 0){
    printf("Check the square braces, curly braces and paranthesis in the code");
  }
  return 0;
}
//it ignores the syntax inside the quote
void quote(int c){
    int nc;
    while ((nc = getchar()) != c ){
        continue;
    }
}
//it ignores the syntax inside the comment.
void comment(int c , int nc){
    int fc,sc;
    if (nc == '/'){
        while((fc=getchar()) != '\n') continue;
        return;
    }
    else{
        sc=getchar();
        while((fc=sc) != '*' && (sc=getchar()) != '/'){
            continue;
        }
        return;
    }
}
