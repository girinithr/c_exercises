/*Exercise 8-7. malloc accepts a size request without checking its plausibility; free believes
that the block it is asked to free contains a valid size field. Improve these routines so they make
more pains with error checking.*/

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

int main(void)
{
        char *c, *d;

        if ((c = (char *) _malloc(sizeof(char) * 1000)) == NULL
         || (d = (char *) _calloc(sizeof(char),  1000)) == NULL) {
                fprintf(stderr, "malloc: returned NULL\n");
                exit(1);
        }
        strcpy(c, "hello,world");
        strcpy(d, "hello,programming");
        printf(" %s\n", c);
        printf(" %s\n", d);
        _free(c);
        _free(d);

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
