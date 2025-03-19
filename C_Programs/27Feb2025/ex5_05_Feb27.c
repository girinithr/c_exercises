/*Exercise 5-5. Write versions of the library functions strncpy, strncat, and strncmp, which
operate on at most the first n characters of their argument strings. For example,
strncpy(s,t,n) copies at most n characters of t to s. Full descriptions are in Appendix B.*/
#include <stdio.h>

#define MAX 20

char *my_strncpy(char *s, const char *t, int n);
char *my_strncat(char *s, const char *t, int n);
int my_strncmp(const char *s, const char *t, int n);

int main(void)
{
	char s[MAX] = "hello";
	char *t = "he";

	/* test my_strncpy */
	my_strncpy(s, t, 5);
	printf("my_strncpy - s = \"%s\"\n", s);
	/* test my_strncat */
	my_strncat(s, t, 20);
	printf("my_strncat - s = \"%s\"\n", s);
	/* test my_strncmp */
	printf("my_strncmp - returns %d comparing \"%s\" with \"%s\"\n",
			my_strncmp(s, t, 4), s, t);

	return 0;
}

char *my_strncpy(char *s, const char *t, int n)
{
	int i;

	for (i = 0; i < n; i++)
		*(s + i) = *(t + i) ? *(t + i) : '\0';

	return s;
}

char *my_strncat(char *s, const char *t, int n)
{
	int i;

	while (*++s)
		;
	for (i = 0; i < n && *(t + i); i++)
		*(s + i) = *(t + i);
	*(s + i) = '\0';

	return s;
}

int my_strncmp(const char *s, const char *t, int n)
{
	int i;

	for (i = 0; i < n && *(s + i) == *(t + i) && *(s + i); i++)
		;
	if (*(s + i) != *(t + i))
		 return (unsigned char)*(s + i) - (unsigned char)*(t + i);

	return 0;
}
