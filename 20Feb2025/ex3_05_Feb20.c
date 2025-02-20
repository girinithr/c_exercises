/*Exercise 3-5. Write the function itob(n,s,b) that converts the integer n into a base b
character representation in the string s. In particular, itob(n,s,16) formats s as a
hexadecimal integer in s.*/
#include<stdio.h>
#include<string.h>

void itob(int n , char s[], int b);
void reverse(char s[]);

#define MAXIMUM 50

int main(){
    int b = 2;
    char s[MAXIMUM];
    itob(123456,s,b);
    printf("Base %d = %s\n",b,s);
    return 0;
}

void itob(int n, char s[], int b){
    int i = 0;
    do
    {
        if (n%b>9)
            s[i++]= n%b +'A'-10;
        else
            s[i++] = n%b +'0';
    } while ((n/=b));
    reverse(s);
}

void reverse (char s[]){
    int i , j , c;
    for (i = 0, j = strlen(s)-1; i < j; i++,j--)
    c = s[i], s[i]=s[j], s[j]=c;
}
