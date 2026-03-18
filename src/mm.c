/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

typedef struct ll_node {
    struct ll_node* prev;
    struct ll_node* next;
} ll_node;

void *heap_listp;
ll_node* dummy_head;
ll_node* dummy_tail;

static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_block(size_t size);
void place(void *bp, size_t asize);

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE               4           /* Size of word and header/footer (bytes) */ 
#define DSIZE               8           /* Double word size */
#define CHUNKSIZE           (1 << 12)   /* Extend heap by this amount (bytes) */

#define MAX(x, y)           ((x) > (y) ? (x) : (y))
#define MIN(x, y)           ((x) < (y) ? (x) : (y))

/* Place a value into an address */
#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)   ((size) | (alloc))

/* Get the size and allocated fields from a header pointer */
#define GET_SIZE(p)         (GET(p) & ~0x7) /* Ignore last 3 bits because 8 byte aligned */
#define GET_ALLOC(p)        (GET(p) & 0x1)

/* From a payload -> calculate the header_ptr or footer_ptr */
#define HDRP(bp)            ((char *)bp - WSIZE) 
#define FTRP(bp)            ((char *)bp + GET_SIZE(HDRP(bp)) - DSIZE) /* Payload size stores size including header and footer, which is why we do - DSIZE */
#define NODE(bp)            ((ll_node *)((char *)bp - sizeof(ll_node) - WSIZE))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the head and tail dummy nodes */
    if ((heap_listp = mem_sbrk((2 * sizeof(ll_node)))) == (void *)-1)
        return -1;
    
    dummy_head = (ll_node*)heap_listp;
    dummy_tail = (ll_node*)(((char*)(heap_listp)) + sizeof(ll_node));
    
    dummy_head->next = dummy_tail;
    dummy_head->prev = NULL;
    dummy_tail->prev = dummy_head;
    dummy_tail->next = NULL;

    /* Extend the heap to have some new large amount of memory to start off with */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    
    return 0;
}

/*
 * extend_heap - extend the heap by adding a block of w words
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Insert a header into the top portion of the block */
    ll_node *prev = dummy_tail->prev;
    bp -= sizeof(ll_node);
    PUT(bp, PACK(size, 0));
    dummy_tail = (ll_node *)(bp + size);
    dummy_tail->next = NULL;
    
    bp = bp + DSIZE;
    
    /* Insert this block at the end of the current heap */
    ll_node* curr = (ll_node*)bp;
    curr->next = dummy_tail;
    curr->prev = prev;
    prev->next = curr;
    dummy_tail->prev = curr;

    //return (void*)(bp + sizeof(ll_node));
    return coalesce((void *)(bp + sizeof(ll_node)));
}

/*
 * coalesce - merge blocks
 */ 
static void *coalesce(void *bp)
{
    char *hdr_ptr = (((char *)bp) - sizeof(ll_node) - DSIZE);
    ll_node *bp_node = (ll_node *)(hdr_ptr + DSIZE);
    ll_node *prev = bp_node->prev;
    ll_node *next = bp_node->next;
    char *prev_hdr = (((char *)prev) - DSIZE);
    char *next_hdr = (((char *)next) - DSIZE);

    /* Check if prev block is dummy_head */
    int can_prev_coalesce = prev == dummy_head || (prev_hdr + GET_SIZE(prev_hdr)) != hdr_ptr ? 0 : 1;

    /* Check if next block is dummy_tail */
    int can_next_coalesce = next == dummy_tail || (hdr_ptr + GET_SIZE(hdr_ptr)) != next_hdr ? 0 : 1;  
    
    if (!can_prev_coalesce && !can_next_coalesce) {
        /* Neither can coalesce */
        return bp;
    } else if (can_prev_coalesce && can_next_coalesce) {
        /* All can coaslesce */
        size_t new_size = GET_SIZE(prev_hdr) + GET_SIZE(hdr_ptr) + GET_SIZE(next_hdr);        
        PUT(prev_hdr, PACK(new_size, 0));

        prev->next = next->next;
        next->next->prev = prev;
        /* Does not matter what is inside the other memory because we are never going to use it anymore - the programmer will */
        return (void *)(prev_hdr + DSIZE + sizeof(ll_node));
    } else if (can_prev_coalesce) {
        /* Only prev */
        size_t new_size = GET_SIZE(prev_hdr) + GET_SIZE(hdr_ptr);        
        PUT(prev_hdr, PACK(new_size, 0));

        prev->next = bp_node->next;
        next->prev = prev;
        return (void *)(prev_hdr + DSIZE + sizeof(ll_node));
    } else if (can_next_coalesce) {
        /* Only next */
        size_t new_size = GET_SIZE(next_hdr) + GET_SIZE(hdr_ptr);        
        PUT(hdr_ptr, PACK(new_size, 0));

        bp_node->next = next->next;
        next->next->prev = bp_node; 
        
        return (void *)(hdr_ptr + DSIZE + sizeof(ll_node));
    } 
    return NULL;
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;       
    
    size_t asize; 
    size_t metadata_size = sizeof(ll_node) + DSIZE; 

    /* Make sure the block_size is a multiple of 8 */
    if (size <= DSIZE)
        asize = metadata_size + DSIZE;
    else
        /* Add DSIZE and DSIZE - 1 because the first is for HDR and FTR */
        asize = DSIZE * ((size + metadata_size + (DSIZE - 1)) / DSIZE);

    char *bp;
    if ((bp = find_block(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
   
    /* Extend the heap because there was no block that could satisfy the request of block_size */ 
    size_t extend_size = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * find_block - Use the best-fit algorithm to find the block that satisfies the request with the
 *      least amount of space used
 */ 
static void *find_block(size_t asize)
{
    ll_node* curr;
    for (curr = dummy_head->next; curr != dummy_tail; curr = curr->next) {
        if (GET_SIZE(((char *)curr) - DSIZE) >= asize) {
            return (void *)(((char *)curr) + sizeof(ll_node));
        }
    }    
    return NULL; /* No fit */
}

/*
 * place - By "placing" the block, we remove it from the explicit free list by manipulating pointers
 *      
 */
void place(void *bp, size_t asize)
{
    /* Gauranteed to have a prev and next block because never can use place() on dummy_head or dummy_tail */
    char* header_ptr = (((char *)bp) - sizeof(ll_node) - DSIZE);
    size_t csize = GET_SIZE(header_ptr);
    size_t min_block_size = sizeof(ll_node) + DSIZE + DSIZE;
    ll_node* curr = (ll_node *)((((char *)bp) - sizeof(ll_node))); 
    if ((csize - asize) < (min_block_size)) {
        /* No split */
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        PUT(header_ptr, PACK(GET_SIZE(header_ptr), 1));
    } else {
       /* Split */ 
        PUT(header_ptr, PACK(asize, 1));

        /* Make new block from split */
        char* new_block = header_ptr + asize;
        PUT(new_block, PACK(csize-asize, 0));
        ll_node *new_block_ptrs = (ll_node *)(new_block + DSIZE);
         
        /* Insert this block into the free list */
        new_block_ptrs->prev = curr->prev;
        new_block_ptrs->next = curr->next;
        curr->prev->next = new_block_ptrs;
        curr->next->prev = new_block_ptrs;
    }
}

/*
 * mm_free - Freeing a block will place it back into the freelist 
 */
void mm_free(void *ptr)
{
    ll_node *curr = (ll_node *)(((char *)ptr) - sizeof(ll_node));
    ll_node *next = NULL;
    char* curr_ptr = (((char *)ptr) - sizeof(ll_node) - DSIZE);
    PUT(curr_ptr, PACK(GET_SIZE(curr_ptr), 0));
    curr_ptr += GET_SIZE(curr_ptr); 
    while (curr_ptr != (char *)dummy_tail) {
        if (!GET_ALLOC(curr_ptr)) {
            next = (ll_node *)(curr_ptr + sizeof(ll_node)); 
            break;
        }
        curr_ptr += GET_SIZE(curr_ptr);
    }

    /* Place it before the tail */
    if (next == NULL)
        next = dummy_tail;
    
    /* Place the curr before next */
    curr->prev = next->prev;
    curr->next = next;
    next->prev->next = curr;   
    next->prev = curr; 
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }       

    if (ptr == NULL) {
        return mm_malloc(size);
    }

    size_t asize; 
    size_t metadata_size = sizeof(ll_node) + DSIZE; 

    /* Make sure the block_size is a multiple of 8 */
    if (size <= DSIZE)
        asize = metadata_size + DSIZE;
    else
        /* Add DSIZE and DSIZE - 1 because the first is for HDR and FTR */
        asize = DSIZE * ((size + metadata_size + (DSIZE - 1)) / DSIZE);

    /* Is the block directly after free and not dummy_tail */
     

    char *hdr_ptr = (((char *)ptr) - sizeof(ll_node) - DSIZE);
    char *next_hdr = hdr_ptr + GET_SIZE(hdr_ptr);
    
    /* Check if next block is dummy_tail */
    int can_next_coalesce = next_hdr == (char *)dummy_tail || GET_ALLOC(next_hdr) || GET_SIZE(next_hdr) + GET_SIZE(hdr_ptr) < asize ? 0 : 1; 
    
    ll_node *curr = (ll_node *)(hdr_ptr + DSIZE);   /* The current block's ll_node */
    ll_node *next = (ll_node *)(next_hdr + DSIZE);  /* The next block's ll_node */

    if (asize <= GET_SIZE(hdr_ptr)) {
        return ptr;
    }
    
    if (can_next_coalesce) {
        size_t old_curr = GET_SIZE(hdr_ptr);
        size_t old_next = GET_SIZE(next_hdr);
        size_t need = asize - old_curr;
        size_t remainder = old_next - need;
        size_t min_block_size = sizeof(ll_node) + DSIZE + DSIZE;

        /* remove next from free list first */
        ll_node *prev_free = next->prev;
        ll_node *next_free = next->next;
        next->prev->next = next->next;
        next->next->prev = next->prev;

        if (remainder < min_block_size) {
            /* absorb whole next block */
            PUT(hdr_ptr, PACK(old_curr + old_next, 1));
        } else {
            /* grow current to exactly asize */
            PUT(hdr_ptr, PACK(asize, 1));

            /* build remainder block immediately after enlarged current */
            char *rem_hdr = hdr_ptr + asize;
            PUT(rem_hdr, PACK(remainder, 0));

            ll_node *rem_node = (ll_node *)(rem_hdr + DSIZE);

            /* put remainder back into free list where next used to be */
            rem_node->prev = prev_free;
            rem_node->next = next_free;
            prev_free->next = rem_node;
            next_free->prev = rem_node;
        }
        return ptr;
    } else {
        /* Extend the heap to have some new large amount of memory to start off with */
        char *bp;
        bp = mm_malloc(size);
        size_t copy_size = MIN(GET_SIZE(hdr_ptr) - sizeof(ll_node) - DSIZE, size);
        bp = memcpy(bp, ptr, copy_size);
        mm_free(ptr);

        return bp;
    }
}
