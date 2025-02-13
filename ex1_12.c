#include <stdio.h>
//program to print the input one word per inline
#define IN 1
#define OUT 0
int main()
{
    int c,state;
    state = OUT;

    while ((c = getchar()) != EOF){
        if (c != ' ' && c != '\n' && c != '\t')
            state = IN;
        else{ 
            state = OUT;//this prints a newline when escape sequence is encountered and waits for the new word
            printf("\n");}
        if (state == IN && c != ' ' && c != '\n')
            putchar(c);//this prints the 
        
    }
    return 0;
}