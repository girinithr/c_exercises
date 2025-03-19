/*Exercise 4-1. Write the function strindex(s,t) which returns the position of the rightmost
occurrence of t in s, or -1 if there is none.*/
#include <stdio.h>
#include <string.h>

int strindex(const char s[], const char t[]);

int main(void)
{
	printf("%d\n", strindex("My name is girinith", "giri"));
	return 0;
}

int strindex(const char s[], const char t[])
{
	for (int i = strlen(s) - 1, j; i >= 0; i--)
	{
		for (j = 0; s[i + j] == t[j] && t[j]; j++);
		if (!t[j])
			return i+1;
	}
	return -1;
}
