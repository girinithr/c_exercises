#include <stdio.h>
//printing fahrenheit to celcius table
int main()
{
    float fahr,cel; //initialisong the fahrenheit and celcius variable
    int upper,lower,step; //initialising the limits of the calculation
    
    upper = 300;
    lower = 0;
    step = 20;
    
    fahr = lower;
    printf("Fahrenheit : Celcius \n");
    while(fahr <= upper){
        /*the 10.0 in %10.0f and the 6.1 in %6.1f is to
        right justify the characters with the headings */
        printf("%10.0f : %6.1f \n",fahr,(5.0/9.0)*(fahr-32.0)); 
        fahr = fahr+step;
    }

    return 0;
}