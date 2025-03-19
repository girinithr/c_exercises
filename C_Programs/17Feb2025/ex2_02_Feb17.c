/* Exercise 2-2. Write a loop equivalent to the for loop above without using && or ||. */
#include <stdio.h>
#include <limits.h>

int main(){
    enum bool { NO, YES };
    int lim = 10000;
    char s[lim];
    int c;
    int i = 0;
    enum bool loop = YES;
    while (loop) {
        if (i >= lim - 1) {
            loop = NO;
        }
        else if ((c = getchar()) == '\n') {
            loop = NO;
        }
        else if (c == EOF) {
            loop = NO;
        }
        else {
            s[i] = c;
            ++i;
        }
    }


    return 0;
    }
