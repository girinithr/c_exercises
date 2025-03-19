/*Exercise 2-8. Write a function rightrot(x,n) that returns the value of the integer x rotated
to the right by n positions.*/
#include <stdio.h>

void print_binary(unsigned x);
int rightrot(int x, int n);

int main() {
    //unsigned x = 0b101010101010;
    int x =25;
    int p = 7, n = 3;

    unsigned result = rightrot(x, n);
    printf("Original x: ");
    print_binary(x);
    printf("\nRotated  x: ");
    print_binary(result);
    printf("\n");

    return 0;
}
int rightrot(x,n){
    for (int i = 0; i< n;i++){
        int bit = x & 1;
         x = (x >> 1) ;
         if (bit << 31)
            x = x | (bit << 31);
        else
            x = x & (0x7fffffff);
    }
        return (x);
}


void print_binary(unsigned x) {
    unsigned mask = 1U << (sizeof(x) * 8 - 1);

    for (int i = 0; i < sizeof(x) * 8; i++) {
        if (x & mask) {
            printf("1");
        } else {
            printf("0");
        }
        x <<= 1;
    }
}
