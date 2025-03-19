/*Exercise 5-9. Rewrite the routines day_of_year and month_day with pointers instead of
indexing.*/
#include <stdio.h>

static char daytab[][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

int day_of_year(int year, int month, int day);
void month_day(int year, int yearday, int *pmonth, int *pday);

int main(void)
{
	int a, b;

	printf("%d\n", day_of_year(2000, 9, 15));

	month_day(2000, 259, &a, &b);
	printf("%d %d\n", a, b);

	return 0;
}

int day_of_year(int year, int month, int day)
{
	int i, leap;
	leap = year % 4 == 0 && year % 100 != 0 || year % 400 == 0;
	if (year >= 1582 && month > 0 && month <= 12 && day > 0 && day <= *(*(daytab + leap) + month))
	{
		for (i = 1; i < month; i++)
			day += *(*(daytab + leap) + i);
		return day;
	}
	else
		return -1;
}

void month_day(int year, int yearday, int *pmonth, int *pday)
{
	int i, leap;
	leap = year % 4 == 0 && year % 100 != 0 || year % 400 == 0;
	if (year >= 1582 && yearday > 0 && (yearday <= 366 && leap || yearday <= 365))
	{
		for (i = 1; yearday > *(*(daytab + leap) + i); i++)
			yearday -= *(*(daytab + leap) + i);
		*pmonth = i;
		*pday = yearday;
	}
	else
		*pmonth = *pday = -1;
}
