/*
 * mm.c - The implementation is done with doubly linked explicit free list. 
 *        The global variable "*free_list" points to bp of Prologue block.
 * 
 * <Allocated block>                                         A = 1 : Allocated
 * --------------------------------------------              A = 0 : Free
 * |             Block size               | A |  Header      (The Width of Rentangle is WSIZE)
 * --------------------------------------------
 * |                                          |
 * |         Payload and Padding              | 
 * |                                          |
 * |                                          |
 * --------------------------------------------
 * |             Block size               | A |  Footer
 * --------------------------------------------
 * 
 * <Free block>
 * --------------------------------------------
 * |             Block size               | A |  Header
 * --------------------------------------------
 * |             Predecessor                  |
 * --------------------------------------------
 * |              Successor                   |
 * --------------------------------------------
 * |                                          | 
 * |                                          |
 * --------------------------------------------
 * |             Block size               | A |  Footer
 * --------------------------------------------
 *  
 * 
 * mm_init - Initialize the malloc package. Set Prologue block and Epilogue block and then Extend heap.
 *
 * mm_malloc - First find free block of which size is equal to or larger than malloc size with first fit algorithm.
 *             If it can find that free block -> allocate
 *                                               if that free block is too large -> Split it
 *			   If it cannot find that free block -> extend heap and allocate
 *
 * mm_free - Set allocated/free flag(A) to zero.
 *           Insert the block to free list and coalesce. (The location of new free block is right after the prologue block.)
 *
 * mm_realloc - mm_malloc for new size.
 *              Copy the memory from old ptr to new ptr.
 *				mm_free the old ptr.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Park Yegi",
    /* First member's email address */
    "pyg9715@kaist.ac.kr",
    /* Second member's full name (leave blank if none) */
    "Eom Sungha",
    /* Second member's email address (leave blank if none) */
    "doubleb@kaist.ac.kr"
};


/* ***** MACROs ***** */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4			    /* Word and header/footer size (bytes) */
#define DSIZE 8			    /* Double word size (bytes) */
#define CHUNKSIZE (1<<12)	/* Extend heap by this amount (bytes)  */

/* Compute maximum b/w x and y */
#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Compute address of predecessor and successor */
#define PRED_PTR(bp) ((char *)(bp))
#define SUCC_PTR(bp) ((char *)(bp) + WSIZE)

/* Compute address of predecessor and successor  */
#define PRED(bp) (*(char **)(bp))
#define SUCC(bp) (*(char **)(SUCC_PTR(bp)))

/* ***** End of MACRO ***** */


/* ***** Global Variables ***** */
void *free_list;


/* ***** Internal Functions ***** */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);
int mm_check(void);

/* 
 * mm_init - Initialize the malloc package.
 *           Set Prologue block and Epilogue block and then Extend heap.
 */
int mm_init(void)
{
	/* Create the initial empty heap */
	if ((free_list = mem_sbrk(8*WSIZE)) == (void *)-1)
		return -1;
	PUT(free_list, 0);							   /* Alignment padding */
	PUT(free_list + (1*WSIZE), PACK(2*DSIZE, 1));  /* Prologue header */
	free_list += (2*WSIZE);						   /* Set free_list to bp of Prologue block */
	PUT(free_list + (2*WSIZE), PACK(2*DSIZE, 1));  /* Prologue footer */
	PUT(free_list + (3*WSIZE), PACK(0, 0));		   /* Epilogue header */

	/* Set predecessor & successor of prologue and epilogue block */
	PUT(PRED_PTR(free_list), NULL);
	PUT(SUCC_PTR(free_list), NEXT_BLKP(free_list));
	PUT(PRED_PTR(NEXT_BLKP(free_list)), free_list);
	PUT(SUCC_PTR(NEXT_BLKP(free_list)), NULL);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *             Always allocate a block whose size is a multiple of the alignment.
 *             
 *             First find free block of which size is equal to or larger than malloc size with first fit algorithm.
 *             If it can find that free block -> allocate
 *                                               if that free block is too large -> Split it
 *			   If it cannot find that free block -> extend heap and allocate
 */
void *mm_malloc(size_t size)
{
	size_t asize;		/* Adjusted block size */
	size_t extendsize;  /* Amount to extend heap if no fit */
	char *bp;

	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL){
		place(bp, asize);
		
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 *
 *           Set allocated/free flag(A) to zero.
 *           Insert the block to free list and coalesce. (The location of new free block is right after the prologue block.)
 */
void mm_free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));

	/* Set allocated/free flag A to zero */
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));

	insert_node(ptr, size);
	coalesce(ptr);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * 
 *              mm_malloc for new size.
 *              Copy the memory from old ptr to new ptr.
 *				mm_free the old ptr.
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *newptr;
	void *oldptr = ptr;
	size_t copySize;
	size_t oldsize = GET_SIZE(HDRP(ptr));

	if (ptr == NULL)
		return mm_malloc(size);
	if (size == 0){
		mm_free(ptr);
		return;
	}

	if ((newptr = mm_malloc(size)) == NULL)
		return NULL;
	memcpy(newptr, oldptr, oldsize < size ? oldsize : size);
	mm_free(oldptr);
	return newptr;
}


/*
 * extend_heap - Extend the size of heap and change the address of epilogue block
 */
static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;
	
	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	bp -= 2*WSIZE;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));	      /* Free block header */
	PUT(FTRP(bp), PACK(size, 0));	      /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	/* Change the address of epilogue block */
	PUT(SUCC_PTR(bp), NEXT_BLKP(bp));
	PUT(PRED_PTR(NEXT_BLKP(bp)), bp);
	PUT(SUCC_PTR(NEXT_BLKP(bp)), NULL);
	
	/* Coalesce if the previous block was free */
	return coalesce(bp);
}

/*
 * coalesce - Join with prev/next block, if they are free
 */
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc){  /* Case 1 - both prev and next are allocated */
		return bp;
	}
	else if (prev_alloc){   		/* Case 2 - prev is allocated / next is free */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		delete_node(NEXT_BLKP(bp));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if (next_alloc){   		/* Case 3 - prev is free / next is allocated */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		delete_node(bp);
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	else {							/* Case 4 - both prev and next is free */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		delete_node(bp);
		delete_node(NEXT_BLKP(bp));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

/*
 * find_fit - Find a free block of which size is equal to or larger than malloc size
 *            Using first find algorithm(searching list from the beginning)
 */
static void *find_fit(size_t asize)
{
	/* First-fit search */
	void *bp;
	
	for (bp = free_list; GET_SIZE(HDRP(bp)) > 0; bp = SUCC(bp)){
		if ((asize <= GET_SIZE(HDRP(bp))) && (bp != free_list)){
			return bp;
		}
	}
	return NULL; /* No fit */
}

/*
 * place - Allocate the asize on the heap where the bp is pointing
 *		   Split the block if the block is bigger enough than asize
 */
static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
	void *prev;
	void *next;

	delete_node(bp);

	/* Split the block */
	if ((csize - asize) >= (2*DSIZE)){ 
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		prev = PRED(bp);
		next = SUCC(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
		PUT(PRED_PTR(bp), prev);
		PUT(SUCC_PTR(bp), next);
		insert_node(bp, csize-asize);
	}
	/* Not split the block */
	else{
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}


/* 
 * insert_node - Insert the block into free list
 *               Place new free block right after the prologue block
 */
static void insert_node(void *ptr, size_t size)
{
	void *insert = free_list;
	void *insert_next = SUCC(insert);

	PUT(SUCC_PTR(insert), ptr);
	PUT(PRED_PTR(insert_next), ptr);
	PUT(PRED_PTR(ptr), insert);
	PUT(SUCC_PTR(ptr), insert_next);

	return;
}

/*
 * delete_node - Delete the block pointed by ptr from free list
 */
static void delete_node(void *ptr)
{
	void *prev = PRED(ptr);
	void *next = SUCC(ptr);
	
	PUT(SUCC_PTR(prev), next);
	PUT(PRED_PTR(next), prev);

	return;
}

/*
 * mm_check - Print all blocks in free list
 *			  Check whether all blocks in free list is free or not
 */
int mm_check(void)
{
	void *bp;
	
	printf("Print bp of blocks in free list\n");
	for (bp = free_list; GET_SIZE(HDRP(bp)) > 0; bp = SUCC(bp)){
		if (bp == free_list)
			printf("Prologue block %p size %x alloc %d\n", bp, GET_SIZE(bp), GET_ALLOC(bp));
		else if (SUCC(bp) == NULL)
			printf("Epilogue block %p size %x alloc %d\n", bp, GET_SIZE(bp), GET_ALLOC(bp));
		else
			printf("free block %p size %x alloc %d\n", bp, GET_SIZE(bp), GET_ALLOC(bp));
	}

	for (bp = free_list; GET_SIZE(HDRP(bp)) > 0; bp = SUCC(bp)){
		if ((GET_ALLOC(HDRP(bp)) != 0) && ((bp != free_list) || (GET_SIZE(HDRP(bp)) == 0))){
			printf("%p : not free block in free list\n", bp);
			return 1;
		}
	}
	
	return 0;
}


