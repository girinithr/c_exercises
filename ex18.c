#include <stdio.h>

int main()
{
    int tab=0,blank=0,n_line=0;
    /*initialising the count values to
    zero otherwise garbage value will be assigned */
    int c;
    
    while((c= getchar()) !=EOF){
        if (c == '\t')
            ++tab; //counts the number of tabs in the given input
        else if (c == '\n')
            ++n_line; //counts the number of newline character in the input
        else if (c == ' ')
            ++blank; //counts the number of blank spaces in the input
    }
    printf("tab:%d  blank:%d    new line:%d",tab,blank,n_line);

    return 0;
}