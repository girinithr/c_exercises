/*Exercise 8-4. The standard library function
int fseek(FILE *fp, long offset, int origin)
is identical to lseek except that fp is a file pointer instead of a file descriptor and return value
is an int status, not a position. Write fseek. Make sure that your fseek coordinates properly
with the buffering done for the other functions of the library.*/

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#ifdef NULL
    #undef NULL
#endif

typedef struct _iobuf
{
    int cnt;    // characters left
    char *ptr;  // next character position
    char *base; // location of buffer
    int flag;   // mode of file access
    int fd;     // file descriptor
} FILE;

enum _flags {
    _READ = 01,  // file open for reading
    _WRITE = 02, // file open for writing
    _UNBUF = 04, // file is unbuffered
    _EOF = 010,  // EOF has occurred on this file
    _ERR = 020   // error occurred on this file
};

#define NULL 0
#define EOF (-1)
#define BUFSIZ 1024
#define OPEN_MAX 20 // max #files open at once
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])

extern FILE _iob[OPEN_MAX];
int _fillbuf(FILE *fp);
int _flushbuf(int c, FILE *fp);

#define feof(p) (((p)->flag & _EOF) == _EOF)
#define ferror(p) (((p)->flag & _ERR) == _ERR)
#define fileno(p) ((p)->fd)
#define getc(p) (--(p)->cnt >= 0 ? (unsigned char) *(p)->ptr++ : _fillbuf(p))
#define putc(x, p) (--(p)->cnt >= 0 ? *(p)->ptr++ = (x) : _flushbuf((x), p))
#define getchar() getc(stdin)
#define putchar(x) putc((x), stdout)
#define PERMS 0666 // RW for owner, group, others
#define MAXERRORMSG 1500

FILE *fopen(char *name, char *mode);
int fflush(FILE *fp);
int fclose(FILE *fp);
void error(char *msg);
int fseek(FILE *fp, long offset, int origin);

FILE _iob[OPEN_MAX] = // stdin, stdout, stderr
{
    { 0, (char *) 0, (char *) 0, _READ, 0 },
    { 0, (char *) 0, (char *) 0, _WRITE, 1 },
    { 0, (char *) 0, (char *) 0, _WRITE | _UNBUF, 2 }
};

int main(int argc, char *argv[])
{
    char msg[MAXERRORMSG];
    FILE *fpIn, *fpOut;
    if (argc == 3)
    {
        if ((fpIn = fopen(*++argv, "r")) == NULL)
            error(strcat(strcat(msg, "error: couldn't open file "), *argv));
        if ((fpOut = fopen(*++argv, "w")) == NULL)
            error(strcat(strcat(msg, "error: couldn't write to file "), *argv));
    }
    else
        error("usage: ./myfseek input_file output_file");

    if (fseek(fpIn, -1, SEEK_END) == EOF)
        error("failed to seek to last char of input file");
    if (fseek(fpOut, 15, SEEK_CUR) == EOF)
        error("failed to seek 15 bytes past the start/end of the output file");
    putc(getc(fpIn), fpOut);

    if (fseek(fpIn, 1, SEEK_SET) == EOF)
        error("failed to seek to first char of input file");
    if (fseek(fpOut, -16, SEEK_END) == EOF)
        error("failed to seek 16 bytes back from the end of the output file (which is the start of the file)");
    putc(getc(fpIn), fpOut);

    fclose(fpIn);
    fclose(fpOut);
    exit(0);
}

FILE *fopen(char *name, char *mode)
{
    int fd;
    FILE *fp;

    if (*mode != 'r' && *mode != 'w' && *mode != 'a')
        return NULL;
    for (fp = _iob; fp < _iob + OPEN_MAX; fp++)
        if ((fp->flag & (_READ | _WRITE)) == 0)
            break;
    if (fp >= _iob + OPEN_MAX) // no free slots
        return NULL;

    if (*mode == 'w')
        fd = creat(name, PERMS);
    else if (*mode == 'a' && (fd = open(name, O_APPEND, 0)) == -1)
        fd = creat(name, PERMS);
    else
        fd = open(name, O_RDONLY, 0);
    if (fd == -1)
        return NULL;
    fp->fd = fd;
    fp->cnt = 0;
    fp->base = NULL;
    fp->flag = (*mode == 'r') ? _READ : _WRITE;
    return fp;
}


int _fillbuf(FILE *fp)
{
    if ((fp->flag & (_READ | _EOF | _ERR)) != _READ)
        return EOF;
    int bufsize = (fp->flag & _UNBUF) ? 1 : BUFSIZ;
    if (fp->base == NULL)
        if ((fp->base = (char *) malloc(bufsize)) == NULL)
            return EOF;
    fp->ptr = fp->base;
    fp->cnt = read(fp->fd, fp->ptr, bufsize);
    if (--fp->cnt < 0)
    {
        if (fp->cnt == -1)
            fp->flag |= _EOF;
        else
            fp->flag |= _ERR;
        fp->cnt = 0;
        return EOF;
    }
    return (unsigned char) *fp->ptr++;
}


int _flushbuf(int c, FILE *fp)
{
    if (fp == NULL)
        return EOF;
    else if (fflush(fp) == EOF)
        return EOF;
    *fp->ptr++ = (char) c;
    fp->cnt--;
    return 0;
}

int fflush(FILE *fp)
{
    if (fp == NULL)
    {
        int result = 0;
        for (int i = 0; i < OPEN_MAX; i++)
            if (((&_iob[i])->flag & _WRITE) == _WRITE && fflush(&_iob[i]) == EOF)
                result = EOF;
        return result;}
    else if (fp < _iob || fp >= _iob + OPEN_MAX)
        return EOF;
    else if ((fp->flag & (_WRITE | _ERR | _READ)) != _WRITE)
        return EOF;

    int bufsize = (fp->flag & _UNBUF) ? 1 : BUFSIZ;
    if (fp->base == NULL)
    {
        if ((fp->base = (char *) malloc(bufsize)) == NULL)
        {
            fp->flag |= _ERR;
            return EOF;
        }
    }
    else
    {
        int n = fp->ptr - fp->base;
        if (write(fp->fd, fp->base, n) != n)
        {
            fp->flag |= _ERR;
            return EOF;
        }
    }
    fp->ptr = fp->base;
    fp->cnt = bufsize;
    return 0;
}


int fclose(FILE *fp)
{
    int result = 0;
    if (fp == NULL || fp < _iob || fp >= _iob + OPEN_MAX)
        return EOF;
    if ((fp->flag & _WRITE) == _WRITE)
        result = fflush(fp);
    if (fp->base != NULL)
        free(fp->base);
    if (close(fp->fd) != 0)
        result = EOF;
    fp->cnt = fp->flag = 0;
    fp->fd = -1;
    fp->ptr = fp->base = NULL;
    return result;
}


void error(char *msg)
{
    fflush(NULL);
    while (*msg != '\0')
        putc(*msg++, stderr);
    putc('\n', stderr);
    fflush(stderr);
    exit(1);
}


int fseek(FILE *fp, long offset, int origin)
{
    if (fp == NULL || fp < _iob || fp >= _iob + OPEN_MAX)
        return EOF;
    else if (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END)
        return EOF;
    else if ((fp->flag & (_READ | _WRITE)) == 0 || (fp->flag & (_READ | _WRITE)) == _READ + _WRITE)
        return EOF;
    if ((fp->flag & _WRITE) == _WRITE)
    {
        if (fflush(fp) == EOF)
            return EOF;
    }
    else
    {
        if (origin == SEEK_CUR)
        {
            if (offset >= 0 && offset <= fp->cnt)
            {
                fp->ptr += offset;
                fp->cnt -= offset;
                return 0;
            }
            offset -= fp->cnt;
        }
        fp->cnt = 0;
    }
    if (lseek(fp->fd, offset, origin) == -1)
    {
        fp->flag |= _ERR;
        return EOF;
    }
    fp->flag &= ~_EOF;
    return 0;
}
