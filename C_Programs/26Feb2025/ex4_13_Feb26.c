/*Exercise 4-13. Write a recursive version of the function reverse(s), which reverses the
string s in place.*/
#include <stdio.h>
/* printd: print n in decimal */
int main(){
    char s[100] = {'g','i','r','i','n','i','t','h','\0'};
    reverse(s);
    int ind = 0;
    int c;
    while(c = s[ind]){
        printf("%c",c);
        ++ind;
    }
}


void reverse(char s[])
{
    static int i;
    static int len;
    len = strlen(s);
    int c, j;
    if(i == 0){
        j = len-1;
    }
    else j = len-(i+1);
    if(i < j) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
        i++;
        reverse(s);
    }
    else
        i = 0;
    return ;
}
