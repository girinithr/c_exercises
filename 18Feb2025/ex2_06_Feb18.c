/*Exercise 2-6. Write a function setbits(x,p,n,y) that returns x with the n bits that begin at
position p set to the rightmost n bits of y, leaving the other bits unchanged.*/
#include <stdio.h>

unsigned setbits(unsigned x, int p, int n, unsigned y);
void print_binary(unsigned x);

int main() {
    unsigned x = 0b101010101010, y = 0b1101;
    int p = 7, n = 3;

    unsigned result = setbits(x, p, n, y);
    print_binary(result);
    return 0;
}

unsigned setbits(unsigned x, int p, int n, unsigned y) {
    // Create a mask to clear the n bits at position p in x
    unsigned mask = ~(~0 << n) << (p + 1 - n);

    // Clear the n bits at position p in x
    x &= ~mask;

    // Shift the rightmost n bits of y to align with position p in x
    y = (y & ~(~0 << n)) << (p + 1 - n);

    // Set the n bits at position p in x to the shifted bits of y
    return x | y;
}

void print_binary(unsigned x) {
    unsigned mask = 1 << (sizeof(x) * 8 - 1);
    for (int i = 0; i < sizeof(x) * 8; i++) {
        if (x & mask) {
            printf("1");
        } else {
            printf("0");
        }
        x <<= 1;
    }
}

