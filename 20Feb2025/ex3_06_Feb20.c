/*Exercise 3-6. Write a version of itoa that accepts three arguments instead of two. The third
argument is a minimum field width; the converted number must be padded with blanks on the
left if necessary to make it wide enough.*/
#include <stdio.h>
#include <string.h>

#define MAXIMUM 50

void itoa(int n, char s[], int width);
void reverse(char s[]);

int main()
{
    char s[MAXIMUM];
    int n = -123456;
    int width = 15;
    itoa(n,s,width);
    printf("%s\n", s);
    return 0;
}

void itoa(int n, char s[], int width)
{
    int i = 0, sign, gap;
    if ((sign = n) < 0)
        n = -n;
    do
    {
        s[i++] = n % 10 + '0';
    } while (n /= 10);
    if (sign<0)
    s[i++]='-';
    gap = width - strlen(s);
    if (gap>0)
    while (gap>0)
    {
        s[i++]=' ';
        gap--;
    }
    else{
        while (gap<0)
        {
            i--;
            s[i]='\b';
            gap++;
        }

    }
    reverse(s);
}

void reverse(char s[]){
    int i,j,c;
    for(i = 0, j = strlen(s)-1; i<j; i++,j--)
        c=s[i],s[i]=s[j],s[j]=c;
}
