#include <stdio.h>

int main()
{
    int c;
    int lc[26];
    int max = 0;
    for (int i=0; i<26; i++){
        lc[i] = 0;   
    }
    while ((c = getchar()) != EOF){
        
        if ( c >= 'a' && c <= 'z'){
            //this if condition is for counting the lower case letters
            lc[c-'a']++;
            max = max>lc[c-'a'] ? max:lc[c-'a']; 
        }
        else if ( c >= 'A' && c <= 'Z'){
            //this condition is for counting the upper case letters
            lc[c-'A']++;
            max = max>lc[c-'A'] ? max:lc[c-'A'];
        }
        if (c == '.') break;//to stop getting input after encountering '.'
    }
    while(max){
        //this double loop is to print the histogram
        for (int i=0;i<26;i++){
            if (lc[i] >= max) printf("* ");
            else printf("  ");
        }
        printf("\n");
        --max;
    }
    /*the below block is to print the alphabets at last to know
    that for which alphabet the histogram corresponds to*/
    c = 'a';
    for (int i=0;i<26;i++){
        printf("%c ",c);
        c++;
    }
    return 0;
}