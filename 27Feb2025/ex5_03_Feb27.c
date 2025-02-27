/*Exercise 5-3. Write a pointer version of the function strcat that we showed in Chapter 2:
strcat(s,t) copies the string t to the end of s.*/
#include <stdio.h>

#define STR_BUFFER 10000

void strcat(char *, char *);

int main(int argc, char *argv[])
{
        char string1[STR_BUFFER] = "What A ";
        char string2[STR_BUFFER] = "Wonderful World!";

        printf ("String 1: %s\n", string1);

        strcat(string1, string2);

        printf ("String 2: %s\n", string2);
        printf ("Cat Result: %s\n", string1);

        return 0;
}

/* Concatenate t to s. */
void strcat(char *s, char *t)
{
        while(*s++); /* Get to the end of the string */
        s--;           /*get back to the end of the string.*/
        while((*s++ = *t++));
}
