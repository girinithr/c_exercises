/*Exercise 3-3. Write a function expand(s1,s2) that expands shorthand notations like a-z in
the string s1 into the equivalent complete list abc...xyz in s2. Allow for letters of either case
and digits, and be prepared to handle cases like a-b-c and a-z0-9 and -a-z. Arrange that a
leading or trailing - is taken literally.*/
#include <stdio.h>
#include <ctype.h>

int isvalid(PREV, NEXT);
void expand(const char s1[], char s2[]);

int main(void)
{
	char *strs[] = {"a-b-c", "a-z0-9", "-a-z", "z-a-", "-1-6-", "a-ee-a", "a-R-L", "1-9-1", "5-5", NULL};
	char result[300];

	for (int i = 0; strs[i]; i++)
	{
		expand(strs[i], result);
		printf("%s => %s\n", strs[i], result);
	}

	return 0;
}

void expand(const char s1[], char s2[])
{
	int j = 0;
	for (int i = 0; s1[i] != '\0'; i++)
	{
		if (s1[i] == '-' && i && isvalid(s1[i - 1], s1[i + 1]))
		{
			j--;
			if (s1[i - 1] <= s1[i + 1])
				for (char c = s1[i - 1]; c <= s1[i + 1]; c++)
					s2[j++] = c;
			else
				for (char c = s1[i - 1]; c >= s1[i + 1]; c--)
					s2[j++] = c;
			i++;
		}
		else
			s2[j++] = s1[i];
	}
	s2[j] = '\0';
}

int isvalid(PREV, NEXT){
    if (islower(PREV) && islower(NEXT) || isupper(PREV) && isupper(NEXT) || isdigit(PREV) && isdigit(NEXT))
        return 1;
    return 0;


}
