/*Exercise 2-7. Write a function invert(x,p,n) that returns x with the n bits that begin at
position p inverted (i.e., 1 changed into 0 and vice versa), leaving the others unchanged..*/
#include <stdio.h>

void print_binary(unsigned x);
unsigned invert(unsigned x, int p, int n);

int main() {
    unsigned x = 0b101010101010;
    int p = 7, n = 3;

    unsigned result = invert(x, p, n);
    printf("Original x: ");
    print_binary(x);
    printf("\nInverted x: ");
    print_binary(result);
    printf("\n");

    return 0;
}

unsigned invert(unsigned x, int p, int n) {
    unsigned mask = ((1U << n) - 1) << (p + 1 - n);

    // XOR the mask with x to invert the bits
    return x ^ mask;
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
