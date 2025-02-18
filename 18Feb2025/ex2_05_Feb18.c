/*Exercise 2-5. Write the function any(s1,s2), which returns the first location in a string s1
where any character from the string s2 occurs, or -1 if s1 contains no characters from s2.
(The standard library function strpbrk does the same job but returns a pointer to the
location.)*/
#include <stdio.h>

int any(char s1[], char s2[]);

int main() {
    char fstr[] = "girinith", sstr[] = "th";

    int result = any(fstr, sstr);

    if (result != -1) {
        printf("First occurrence at index: %d\n", result);
    } else {
        printf("No characters from s2 found in s1.\n");
    }

    return 0;
}

int any(char s1[], char s2[]) {
    int i = 0, j = 0;

    // Loop through each character in s1
    while (s1[i]) {
        // Check if s1[i] is in s2
        for (j = 0; s2[j]; j++) {
            if (s1[i] == s2[j]) {
                return i;  // Return the index of first match
            }
        }
        i++;
    }

    return -1;  // Return -1 if no characters from s2 are found in s1
}
