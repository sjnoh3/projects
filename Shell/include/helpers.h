#ifndef HELPERS_H
#define HELPERS_H

#include "icsmm.h"
#include <string.h>

/* Helper function declarations go here */
#define WSIZE 8			/* Word / Header / Footer size in bytes */
#define DSIZE 16		/* Double word size in bytes */
#define PSIZE (1<<12)	/* Page Size in bytes - 4096 bytes */

void setFreeHeader(ics_free_header *newFree, ics_free_header *prev, ics_free_header *next, uint64_t block_size, uint64_t padding_amount);
void setEpilogue(ics_header **epilogue);
void setFooter(ics_header *header);
void allocMem(void **hptr, ics_free_header **freelist_head, ics_free_header **freeBlk, size_t size, uint64_t *padByte);
void insertInOrder(ics_free_header **freelist_head, ics_free_header **newNode);
void removeNode(ics_free_header **freelist_head, ics_free_header *temp);

#endif
