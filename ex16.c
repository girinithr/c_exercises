#include <stdio.h>
//veifying that "getchar() != EOF" always evaluates to 0 or 1
int main()
{   int c;
    c = getchar() != EOF;
    printf("%d",c);

    return 0;
}