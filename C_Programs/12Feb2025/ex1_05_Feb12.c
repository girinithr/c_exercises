#include <stdio.h>
//printing celcius to fahrenheit table
int main()
{
    float fahr,cel; //initialisong the fahrenheit and celcius variable
    int upper,lower,step; //initialising the limits of the calculation
    
    upper = 300;
    lower = 0;
    step = 20;
    
    cel = upper;
    printf("Celcius : Fahrenheit \n");
    while(cel >= lower){
        /*the 7.0 in %7.0f and the 9.1 in %9.1f is to
        right justify the characters with the headings */
        printf("%7.0f : %9.1f \n",cel,(9.0/5.0)*(cel+32.0)); 
        cel = cel-step;
    }

    return 0;
}