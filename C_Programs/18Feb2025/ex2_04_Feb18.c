/*Exercise 2-4. Write an alternative version of squeeze(s1,s2) that deletes each character in
s1 that matches any character in the string s2.*/
#include <stdio.h>

char* squeeze(char s1[], char s2[]);

int main() {
    char fstr[] = "girinith", sstr[] = "th";

    printf("%s", squeeze(fstr, sstr));

    return 0;
}

char* squeeze(char s1[], char s2[]) {
    int arr[256] = {0};  // Array to store occurrences of characters
    int i = 0, j = 0, k = 0;

    // Mark all characters in s2 in the array `arr`
    while (s2[i]) {
        arr[s2[i]] = 1;  // Set to 1 if character exists in s2
        i++;
    }

    // Iterate over s1 and remove characters that exist in s2
    while (s1[j]) {
        if (!arr[s1[j]]) {  // If the character is not in s2
            s1[k++] = s1[j];  // Keep the character in s1
        }
        j++;
    }

    s1[k] = '\0';  // Null-terminate the result string

    return s1;
}
