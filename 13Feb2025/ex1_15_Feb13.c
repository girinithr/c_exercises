#include <stdio.h>
//printing fahrenheit to celcius table
float temp_converter(int fahr);//function prototype
int main()
{
    int fahr = 20;
    printf("The degree celcius equivalent to %d fahrenheit is %f",fahr,temp_converter(fahr));

    return 0;
}
//function definition
float temp_converter(int fahr){
    float celc;
    
    celc = (5.0/9.0) * (fahr - 32.0);
    return celc;
    
}