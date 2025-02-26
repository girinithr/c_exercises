/*Exercise 4-12. Adapt the ideas of printd to write a recursive version of itoa; that is, convert
an integer into a string by calling a recursive routine.*/
#include <stdio.h>
/* printd: print n in decimal */
int main(){
    char result[100];
    int n = -12354;
    itoa(result,n);
    int ind= 0;
    int c;
    while(c=result[ind]){
        printf("%c",c);
        ++ind;
    }
}


void itoa(char result[],int n)
{   static int ind = 0;
    if (n < 0) {
        result[ind++] = '-' ;
        n = -n;
    }
    if (n / 10)
        itoa(result,n / 10);
    result[ind++] = n % 10 + '0';
}
