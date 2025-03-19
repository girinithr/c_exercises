/*Exercise 7-9. Functions like isupper can be implemented to save space or to save time.
Explore both possibilities.*/
int my_isupper(int c);

int main(){
    int a;
    a = 'Q';
    if(my_isupper(a))
        printf("%c is a uppercase letter",a);
    else
        printf("%c is a lowercase letter",a);
    return 0;
}
int my_isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}
