/*Exercise 2-10. Rewrite the function lower, which converts upper case letters to lower case,
with a conditional expression instead of if-else.*/
#include <stdio.h>

int lower(int c);

int main() {
    int c;
    c = 'A';
    printf("%c",lower(c));
    return 0;
}
int lower(int c)
{   //replacing if-else with ternary operator
    return (c >= 'A' && c <= 'Z') ? c + 'a' - 'A': c;

}

