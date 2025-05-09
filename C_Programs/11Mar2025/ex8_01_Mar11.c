/*Exercise 8-1. Rewrite the program cat from Chapter 7 using read, write, open, and close
instead of their standard library equivalents. Perform experiments to determine the relative
speeds of the two versions.*/

#include <stdio.h>
#include <fcntl.h>
//#include <syscalls.h>
#define PERMS 0666
void error(char *,...);
/* RW for owner, group, others */

/* cp: copy f1 to f2 */
main(int argc, char *argv[])
{
    int f1, f2, n;
    char buf[BUFSIZ];
    if (argc != 3)
        error("Usage: cp from to");
    if ((f1 = open(argv[1], O_RDONLY, 0)) == -1)
        error("cp: can't open %s", argv[1]);
    if ((f2 = open(argv[2], O_WRONLY | O_APPEND , 0)) == -1)
        error("cp: can't create %s, mode %03o",
        argv[2], PERMS);
    while ((n = read(f1, buf, BUFSIZ)) > 0)
        if (write(f2, buf, n) != n)
            error("cp: write error on file %s", argv[2]);
    return 0;
}
