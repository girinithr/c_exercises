/*Exercise 1-23. Write a program to remove all comments from a C program. Don't forget to
 handle quoted strings and character constants properly. C comments don't nest.
*/
#include <stdio.h>

void quote(int c);
void comment(int c, int nc);

int main(void)
{

  int c,nc;


  while ((c = getchar()) != EOF)
  {
    if (c == '\'' || c == '"') quote(c);
    else if (c == '/'){
        nc  = getchar();
        if (nc == '*' || nc == '/')
            comment(c,nc);
    }
    else{
        putchar(c);
    }
  }

  return 0;
}
void quote(int c){
    int nc;
    while ((nc = getchar()) != c ){
        if (nc == '\\'){
            putchar(getchar());
        }
        else{
            putchar(nc);
        }
    }
}
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
