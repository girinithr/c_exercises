/*Exercise 8-8. Write a routine bfree(p,n) that will free any arbitrary block p of n characters
into the free list maintained by malloc and free. By using bfree, a user can add a static or
external array to the free list at any time.*/

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define NALLOC 1024


typedef long Align;

union header {
        struct {
                union header *ptr;
                unsigned size;
        } s;
        Align x;
};

typedef union header Header;

static Header base;
static Header *freep = NULL;

static void *_malloc(unsigned nbytes);
static void *_calloc(unsigned nbytes, unsigned size);
static Header *morecore(unsigned nu);
static void _free(void *ap);
static void bfree(void *p, unsigned n);

int main(void)
{
        char *c, *d, *f;;
        char e[1096];

        if ((c = (char *) _malloc(sizeof(char) * 1000)) == NULL
         || (d = (char *) _calloc(sizeof(char),  1000)) == NULL) {
                fprintf(stderr, "malloc: returned NULL\n");
                exit(1);
        }
        strcpy(c, "hello,world");
        strcpy(d, "hello,programming");
        printf("\%s\n", c);
        printf("%s\n", d);
        _free(c);
        _free(d);

        printf(" array is at @0x%p\n", e);
        bfree(e, 1096);
        if ((f = (char *) _calloc(sizeof(char), 1024)) == NULL) {
                fprintf(stderr, "malloc: returned NULL\n");
                exit(1);
        }
        printf("calloc is at @0x%p\n", f);
        strcpy(f, "hello,user");
        printf("%s\n", f);
        _free(f);

        return 0;
}

static void *_calloc(unsigned nbytes, unsigned size)
{
        void *p;
        unsigned t = nbytes * size;

        if ((p = _malloc(t)) == NULL)
                return NULL;
        memset(p, 0, t);
        return p;
}

static void *_malloc(unsigned nbytes)
{
        Header *p, *prevp;
        Header *morecore(unsigned);
        unsigned nunits;

        if (nbytes == 0 || nbytes >= UINT_MAX - NALLOC) {
                fprintf(stderr, "malloc: invalid size %u\n", nbytes);
                return NULL;
        }
        nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
        if ((prevp = freep) == NULL) {
                base.s.ptr = freep = prevp = &base;
                base.s.size = 0;
        }
        for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
                if (p->s.size >= nunits) {
                        if (p->s.size == nunits)
                                prevp->s.ptr = p->s.ptr;
                        else {
                                p->s.size -= nunits;
                                p += p->s.size;
                                p->s.size = nunits;
                        }
                        freep = prevp;
                        return (void *)(p+1);
                }
                if (p == freep)
                        if ((p = morecore(nunits)) == NULL)
                                return NULL;
        }
}

static Header *morecore(unsigned nu)
{
        char *cp;
        Header *up;

        if (nu < NALLOC)
                nu = NALLOC;
        cp = sbrk(nu * sizeof(Header));
        if (cp == (char *) -1)
                return NULL;
        up = (Header *) cp;
        up->s.size = nu;
        _free((void *)(up+1));
        return freep;
}

static void _free(void *ap)
{
        Header *bp, *p;

        bp = (Header *)ap - 1;
        if (bp->s.size == 0 || bp->s.size == UINT_MAX - NALLOC) {
                fprintf(stderr, "free: invalid block size %u\n", bp->s.size);
                return;
        }
        for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
                if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
                        break;
        if (bp + bp->s.size == p->s.ptr) {
                /*  we should check for UINT_MAX overflow before merge */
                bp->s.size += p->s.ptr->s.size;
                bp->s.ptr = p->s.ptr->s.ptr;
        } else
                bp->s.ptr = p->s.ptr;
        if (p + p->s.size == bp) {
                /*  we should check for UINT_MAX overflow before merge */
                p->s.size += bp->s.size;
                p->s.ptr = bp->s.ptr;
        } else
                p->s.ptr = bp;
        freep = p;
}

static void bfree(void *p, unsigned n)
{
        Header *bp;

        if (n < NALLOC) {
                fprintf(stderr, "bfree: block is to short, %u must be at least %u\n", n, NALLOC);
                return;
        }
        bp = (Header *) p;
        /* We must substract 1 sizeof(Header) to size for Header space */
        bp->s.size = (n / sizeof(Header)) - 1;
        _free((void *)(bp + 1));
}
