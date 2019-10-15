/*
 * mm_dlink_kr_heap.c
 *
 * Based on C dynamic memory manager code from
 * Brian Kernighan and Dennis Richie (K&R)
 *
 *  @since July 24, 2019
 *  @author philip gust & luna szymanski
 */


#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include "memlib.h"
#include "mm_heap.h"

typedef union Header {          /* block header/footer */
    struct {
        size_t blksize: 8*sizeof(size_t)-1;	 // size of this block including header+footer
                                            // measured in multiples of header size;
        size_t isalloc: 1;                  // 1 if block allocated, 0 if free
    } s;
    union Header *blkp;                     // pointer to adjacent block on free list //TODO when do i use this
    max_align_t _align;                     // force alignment to max align boundary
} Header;


// forward declarations
static Header *morecore(size_t);
void visualize(const char*);
static size_t mm_bytes(size_t);

/** Pointer to base which is my dummy*/
static Header *base = NULL;

/** Start of memory list */
static Header *freep = NULL;

/**
 * Initialize memory allocator
 */
void mm_init() {
	mem_init();
    //adding base to heap
	//4 units because that is my blksize but base is NULL for now
    size_t nbytes = mm_bytes(4); // number of bytes for header
    base = (Header*)mem_sbrk(nbytes);

	//set up first block - size of 4 Headers is minimum
    base->s.blksize = 4;
    base->s.isalloc = 1;

    //set up my prev and next ptrs
    base[1].blkp = base;
    base[2].blkp = base;

    //set up footer
    base[3].s.blksize =4;
    base[3].s.isalloc = 1;

    //set freep
    freep=base;
}

/**
 * Reset memory allocator.
 */
void mm_reset() {
	mem_reset_brk();
	   //adding base to heap
		//4 units because that is my blksize but base is NULL for now
	    size_t nbytes = mm_bytes(4); // number of bytes for header
	    base = mem_sbrk(nbytes);

		//set up first block - size of 4 Headers is minimum
	    base->s.blksize = 4;
	    base->s.isalloc = 1;

	    base[1].blkp = base;
	    base[2].blkp = base;

	    //set up footer
	    base[3].s.blksize =4;
	    base[3].s.isalloc = 1;

	    //set freep
	    freep=base;
}

/**
 * De-initialize memory allocator.
 */
void mm_deinit() {
	mem_deinit();
    base=freep=NULL;
}

/**
 * Allocation units for nbytes bytes.
 *
 * @param nbytes number of bytes
 * @return number of units for nbytes
 */
inline static size_t mm_units(size_t nbytes) {
    /* smallest count of Header-sized memory chunks */
    /*  (+1 additional chunk for the Header itself) needed to hold nbytes */
    return (nbytes + sizeof(Header) - 1) / sizeof(Header) + 4;
}

/**
 * Allocation bytes for nunits allocation units.
 *
 * @param nunits number of units
 * @return number of bytes for nunits
 */
inline static size_t mm_bytes(size_t nunits) {
    return nunits * sizeof(Header);
}

/**
 * Get pointer to block payload.
 *
 * @param bp the block
 * @return pointer to allocated payload
 */
inline static void *mm_payload(Header *bp) {
	return bp + 3;
}

/**
 * Get pointer to block for payload.
 *
 * @param ap the allocated payload pointer
 */
inline static Header *mm_block(void *ap) {
	return (Header*)ap - 3;
}


/**
 * Allocates size bytes of memory and returns a pointer to the
 * allocated memory, or NULL if request storage cannot be allocated.
 *
 * @param nbytes the number of bytes to allocate
 * @return pointer to allocated memory or NULL if not available.
 */
void *mm_malloc(size_t nbytes) {
    if (freep == NULL) {
    	//need to make my dummy on my heap
    	mm_init();
    }
    //always start at beginning so that i know I am not adding unless
    //i have searched the entire list
    Header *prevp = base;
    size_t nunits = mm_units(nbytes);

    //while my next pointer isn't my dummy
    while(true) {
		if ((prevp->s.blksize >= nunits) && (prevp->s.isalloc == 0)) {
			//my splitting section is commented out because it is crashing
			//my program and I am not able to see if the problem is caused
			//in this split or outside of it
			if (prevp->s.blksize > nunits) {
//				prevp->s.blksize -=nunits-4;
//				Header *upper = prevp;
//				upper = upper + prevp->s.blksize;
//				upper->s.blksize = nunits-4;
//				//link in
//				upper[2].blkp = prevp[2].blkp;
//				prevp[2].blkp = upper;
//				upper[1].blkp = prevp;
//				upper[2].blkp[1].blkp = upper;
				prevp->s.blksize = nunits;
			}
			//block is big enough and empty
			prevp->s.isalloc = 1;
			freep->blkp = prevp; //why does it crash if this is just freep=prevp
			return mm_payload(prevp);
		} else {
			//block is too small or full
			//need more memory now
			if (prevp[2].blkp == base) {
				 //need to make more memory
				 Header *add = morecore(nunits);
				 if (add == NULL) {
					errno = ENOMEM;
					return NULL;                /* none left */
				 }
				 add->s.isalloc=1;
				 //link my new added node to my list
				 add[1].blkp=prevp;
				 add[2].blkp=base;
				 prevp[2].blkp=add;
				 base[1].blkp=add;

				 freep=add;
				 return mm_payload(add);
			}
			//just move to next if next is not my base
			prevp= prevp[2].blkp;

		}
    }

	assert(false); //shouldnt get here

}


/**
 * Deallocates the memory allocation pointed to by ap.
 * If ap is a NULL pointer, no operation is performed.
 *
 * @param ap the memory to free
 */
void mm_free(void *ap) {
	// ignore null pointer
    if (ap == NULL) {
        return;
    }

    Header *bp = mm_block(ap);   /* point to block header */
    if (bp->s.isalloc == 1) {
    	bp->s.isalloc = 0;
    }
//    bp[2]->blkp = freep[2].blkp;
//    freep[2]->blkp[1] = bp;
//    bp[1]->blkp = freep;
//    freep[2]->blkp = bp;



    freep=bp;

    //coalescnce code is commented out because it causes a crash and I am not
    //sure if the problem is here or outside of it. It seems like my *bp
    //does not have its prev and next pointers properly setup
    //get lower neighbor
//    Header *prevblock = bp[1].blkp;
//    if (prevblock->s.isalloc == 0) {
//    	 //coalesce time
//    	prevblock->s.blksize += bp->s.blksize;
//    	prevblock[2].blkp = bp[2].blkp;
//    	freep->blkp = prevblock;
//    }
//
//    Header *nextblock = bp[2].blkp;
//    if (nextblock->s.isalloc == 0 ){
//    	//coalesce with upper neighbor
//    	bp->s.blksize += nextblock->s.blksize;
//    	bp[2].blkp = nextblock[2].blkp;
//    	freep->blkp = bp;
//    }
}

/**
 * Tries to change the size of the allocation pointed to by ap
 * to size, and returns ap.
 *
 * If there is not enough room to enlarge the memory allocation
 * pointed to by ap, realloc() creates a new allocation, copies
 * as much of the old data pointed to by ptr as will fit to the
 * new allocation, frees the old allocation, and returns a pointer
 * to the allocated memory.
 *
 * If ap is NULL, realloc() is identical to a call to malloc()
 * for size bytes.  If size is zero and ptr is not NULL, a minimum
 * sized object is allocated and the original object is freed.
 */
void* mm_realloc(void *ap, size_t newsize) {
	// NULL ap acts as malloc for size newsize bytes
	if (ap == NULL) {
		return mm_malloc(newsize);
	}

	Header* bp = mm_block(ap);    // point to block header
	if (newsize > 0) {
		// return this ap if allocated block large enough
		if (mm_bytes(bp->s.blksize) >= newsize) {
			return ap;
		}
	}

	// allocate new block
	void *newap = mm_malloc(newsize);
	if (newap == NULL) {
		return NULL;
	}
	// copy old block to new block
	size_t oldsize = mm_bytes(bp->s.blksize-4);
	memcpy(newap, ap, (oldsize < newsize) ? oldsize : newsize);
	mm_free(ap);
	return newap;
}


/**
 * Request additional memory to be added to this process.
 *
 * @param nu the number of Header-chunks to be added
 */
static Header *morecore(size_t nu) {
	// nalloc based on page size
	size_t nalloc = mem_pagesize()/sizeof(Header);

    /* get at least NALLOC Header-chunks from the OS */
    if (nu < nalloc) {
        nu = nalloc;
    }

    size_t nbytes = mm_bytes(nu); // number of bytes
    void* p = mem_sbrk(nbytes);
    if (p == (char *) -1) {	// no space
        return NULL;
    }

    Header* bp = (Header*)p;
    bp->s.blksize = nu;

    // add new space to the circular list
    mm_free(bp+1);

    return freep;
}

/**
 * Print the free list (debugging only)
 *
 * @msg the initial message to print
 */
void visualize(const char* msg) {
    Header* tmp;

    fprintf(stderr, "\n--- Free list after \"%s\":\n", msg);

    if (freep == NULL) {                   /* does not exist */
        fprintf(stderr, "    List does not exist\n\n");
        return;
    }

    if (freep == base) {          /* self-pointing list = empty */
        fprintf(stderr, "    List is empty\n\n");
        return;
    }

    tmp = freep;                           /* find the start of the list */
    char* str = "    ";
    do {           /* traverse the list */
        fprintf(stderr, "%sptr: %10p size: %-3I64u\n", str, (void *)tmp, (size_t)tmp->s.blksize);
        str = " -> ";
        tmp = tmp[2].blkp;
    }  while (tmp[2].blkp > freep);

    fprintf(stderr, "--- end\n\n");
}


/**
 * Calculate the total amount of available free memory.
 *
 * @return the amount of free memory in bytes
 */
size_t mm_getfree(void) {
    if (freep == NULL) {
        return 0;
    }

    Header *tmp = freep;
    size_t res = tmp->s.blksize;

    while (tmp->blkp > tmp) {
        tmp = tmp[2].blkp;
        res += tmp->s.blksize;
    }

    return mm_bytes(res);
}
