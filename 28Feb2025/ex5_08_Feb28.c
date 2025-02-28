/*Exercise 5-8. There is no error checking in day_of_year or month_day. Remedy this defect.*/
#include <stdio.h>

/*
    Exercise 5-8. There is no error checking in day_of_year or month_day. Remedy this defect.
*/

int day_of_year(int year, int month, int day);
void month_day(int year, int yearday, int *pmonth, int *pday);

static char daytab[2][13] = {
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

int main()
{
    int m, d;
    month_day(2021, 366, &m, &d);
    printf("%d %d\n", m, d);
    month_day(2020, 366, &m, &d); // valid
    printf("%d %d\n", m, d);
    month_day(2021, -1, &m, &d);
    printf("%d %d\n", m, d);
    month_day(2021, 0, &m, &d);
    printf("%d %d\n", m, d);
    month_day(-2020, 60, &m, &d); // valid
    printf("%d %d\n", m, d);

    printf("\n");

    printf("%d\n", day_of_year(2021, 2, 29));
    printf("%d\n", day_of_year(2020, 2, 29)); // valid
    printf("%d\n", day_of_year(2021, 1, 32));
    printf("%d\n", day_of_year(2021, 1, 31)); // valid
    printf("%d\n", day_of_year(2021, 4, 31));
    printf("%d\n", day_of_year(2021, 4, 30)); // valid
    printf("%d\n", day_of_year(2021, 0, 1));
    printf("%d\n", day_of_year(2021, -1, 1));
    printf("%d\n", day_of_year(2021, 1, 0));
    printf("%d\n", day_of_year(2021, 1, -1));
    printf("%d\n", day_of_year(2021, 13, 1));
    printf("%d\n", day_of_year(-2020, 2, 29)); // valid

    return 0;
}

// set day of year from month and day. Returns 0 if input is invalid
int day_of_year(int year, int month, int day)
{
    int leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; // if leap year, value is 1, otherwise 0 since arithmetic value of a logical expression is 0 for false and 1 for true
    if (day < 1 || month < 1 || month > 12 || (month == 2 && leap && day > 29) || (month == 2 && !leap && day > 28) || ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) || (!(month == 4 || month == 6 || month == 9 || month == 11) && day > 31))
        return 0; // invalid input
    for (int i = 1; i < month; i++)
        day += daytab[leap][i];
    return day;
}

// set month, day from day of year. Sets pmonth and pday to zero if input is invalid
void month_day(int year, int yearday, int *pmonth, int *pday)
{
    int i;
    int leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; // if leap year, value is 1, otherwise 0
    if (yearday < 1 || (leap && yearday > 366) || (!leap && yearday > 365))
    {
        *pmonth = *pday = 0;
        return; // invalid input
    }
    for (i = 1; yearday > daytab[leap][i]; i++)
        yearday -= daytab[leap][i];
    *pmonth = i;
    *pday = yearday;
}
