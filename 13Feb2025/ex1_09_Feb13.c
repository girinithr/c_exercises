#include <stdio.h>
//program to remove duplicate blank spaces
int main()
{
    int flag = 0;
    int c;
    while((c= getchar()) != EOF){
        if (flag == 1 && c != ' ')
            flag = 0; //here the flag is agin set to 0 when the non blank character is encounterd
        if (flag == 0){
        /*this logic prevents the duplicate blank spaces
        by setting the flag to 1 when the first blank space is encountered*/
            putchar(c);
            if (c == ' ') flag = 1;
        }
    }

    return 0;
}