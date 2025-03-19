/*Exercise 8-2. Rewrite fopen and _fillbuf with fields instead of explicit bit operations. Compare code size and execution speed.*//*Exercise 8-2. Rewrite fopen and _fillbuf with fields instead of explicit bit operations.
Compare code size and execution speed.*/
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef NULL
    #undef NULL
#endif

typedef struct _iobuf
{
    int cnt;     // characters left
    char *ptr;   // next character position
    char *base;  // location of buffer
    struct flags // mode of file access field struct
    {
        unsigned int _READ : 1;  // file open for reading
        unsigned int _WRITE : 1; // file open for writing
        unsigned int _UNBUF : 1; // file is unbuffered
        unsigned int _EOF : 1;   // EOF has occurred on this file
        unsigned int _ERR : 1;   // error occurred on this file
    } _flags;
    int fd;     // file descriptor
} FILE;

#define NULL 0
#define EOF (-1)
#define BUFSIZ 1024
#define OPEN_MAX 20 // max #files open at once
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])

extern FILE _iob[OPEN_MAX];
int _fillbuf(FILE *fp);
int _flushbuf(int, FILE *);

#define feof(p) ((p)->_flags._EOF != 0)
#define ferror(p) ((p)->_flags._ERR != 0)
#define fileno(p) ((p)->fd)
#define getc(p) (--(p)->cnt >= 0 ? (unsigned char) *(p)->ptr++ : _fillbuf(p))
#define putc(x, p) (--(p)->cnt >= 0 ? *(p)->ptr++ = (x) : _flushbuf((x), p))
#define getchar() getc(stdin)
#define putchar(x) putc((x), stdout)
#define PERMS 0666 // RW for owner, group, others

FILE _iob[OPEN_MAX] = // stdin, stdout, stderr
{
    { 0, (char *) 0, (char *) 0, { ._READ = 1 }, 0 },
    { 0, (char *) 0, (char *) 0, { ._WRITE = 1 }, 1 },
    { 0, (char *) 0, (char *) 0, { ._WRITE = 1, ._UNBUF = 1 }, 2 }
};
FILE *fopen(char *name, char *mode);

int main(int argc, char *argv[])
{
    FILE *fp;
	char c;

	while (--argc > 0)
		if ((fp = fopen(*++argv, "r")) == NULL){
            printf("File is not present");
			return 1;}
		else
			printf("File is present");
	return 0;
}

// open file, return file ptr
FILE *fopen(char *name, char *mode)
{
    int fd;
    FILE *fp;

    if (*mode != 'r' && *mode != 'w' && *mode != 'a')
        return NULL;
    for (fp = _iob; fp < _iob + OPEN_MAX; fp++)
        if (fp->_flags._READ == 0 && fp->_flags._WRITE == 0) // if both flags are zero
            break;
    if (fp >= _iob + OPEN_MAX) // no free slots
        return NULL;

    if (*mode == 'w')
        fd = creat(name, PERMS);
    else if (*mode == 'a')
    {
        if ((fd = open(name, O_WRONLY, 0)) == -1)
            fd = creat(name, PERMS);
        lseek(fd, 0L, 2);
    }
    else
        fd = open(name, O_RDONLY, 0);
    if (fd == -1) // couldn't access name
        return NULL;
    fp->fd = fd;
    fp->cnt = 0;
    fp->base = NULL;
    fp->_flags._READ = fp->_flags._WRITE = fp->_flags._UNBUF = fp->_flags._EOF = fp->_flags._ERR = 0;
    if (*mode == 'r')
        fp->_flags._READ = 1;
    else
        fp->_flags._WRITE = 1;
    return fp;
}

// allocate and fill input buffer
int _fillbuf(FILE *fp)
{
    int bufsize;
    if (fp->_flags._READ == 0 || fp->_flags._EOF == 1 || fp->_flags._ERR == 1)
        return EOF;
    bufsize = (fp->_flags._UNBUF == 1) ? 1 : BUFSIZ;
    if (fp->base == NULL) // no buffer yet
        if ((fp->base = (char *) malloc(bufsize)) == NULL)
            return EOF; // can't get buffer
    fp->ptr = fp->base;
    fp->cnt = read(fp->fd, fp->ptr, bufsize);
    if (--fp->cnt < 0)
    {
        if (fp->cnt == -1)
            fp->_flags._EOF = 1;
        else
            fp->_flags._ERR = 1;
        fp->cnt = 0;
        return EOF;
    }
    return (unsigned char) *fp->ptr++;
}
