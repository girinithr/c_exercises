#include <stdio.h>
//program to print the input one word per inline
#define IN 1
#define OUT 0
int main()
{
    int c,state,wc,lc=0;
    int wl[10];
    state = OUT;
    int max = 0;
    wc = 0;
    while ((c = getchar()) != EOF){

        if (c != ' ' && c != '\n' && c != '\t')
            state = IN;
        else{
            state = OUT;
            wl[wc++] = lc;
            max = max>lc ? max : lc ;
            lc =0;
        }
        if (state == IN && c != ' ' && c != '\n')
            ++lc;
        if (c == '.') break;//to stop getting input after encountering '.'
    }

    while(max){
        //this double loop is to print the histogram
        for (int i=0;i<wc;i++){
            if (wl[i] >= max) printf("* ");
            else printf("  ");
        }
        printf("\n");
        --max;
        }

    return 0;
}
