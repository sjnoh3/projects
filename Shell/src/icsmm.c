/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 * If you want to make helper functions, put them in helpers.c
 */
#include "icsmm.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define WSIZE 8			/* Word / Header / Footer size in bytes */
#define DSIZE 16		/* Double word size in bytes */
#define PSIZE (1<<12)	/* Page Size in bytes - 4096 bytes */


ics_free_header *freelist_head = NULL;
void *hptr = NULL;
ics_footer *prologue = NULL;
ics_header *epilogue = NULL;
int pageCount = 0;

void *ics_malloc(size_t size) 
{
	uint64_t padByte = 0;
	if(size == 0)
	{
		errno = EINVAL;
		return NULL;
	}

	if(size % DSIZE != 0)
	{
		padByte = DSIZE - (size % DSIZE);
	}

	// FL is Empty
	if(!freelist_head)
	{
		// First inc_malloc call
		if(pageCount == 0)
		{
			hptr = ics_inc_brk();
			// Prologue
			prologue = hptr;
			prologue->block_size = 1;
			prologue->fid = FID;
			// Epilogue
			setEpilogue(&epilogue);
			// Set free block header
			freelist_head = hptr + WSIZE;		
			setFreeHeader(freelist_head, NULL, NULL, 4080, 0);
		}
		// All 4 pages of memory are full
		else if(pageCount == 4)
		{
			errno = ENOMEM;
			return NULL;
		}
		else
		{
			hptr = ics_inc_brk();
			setEpilogue(&epilogue);
			hptr -= WSIZE;
			freelist_head = hptr;
			setFreeHeader(freelist_head, NULL, NULL, PSIZE, 0);
		}
		pageCount++;		
	}

	size_t reqBlkSize = size + padByte + DSIZE;
	ics_free_header *holdHead = freelist_head;
	// FL consists only one node
	if(freelist_head->next == NULL)
	{
		// Free block is too small for the malloc request.
		if(reqBlkSize > freelist_head->header.block_size)
		{
			// pageCount is at 4. No more memory available.
			if(pageCount == 4)
			{
				errno = ENOMEM;
				return NULL;
			}
			else
			{
				/* 1. Request for another page of memory */
				/* 2. Move epilogue */
				/* 3. Create a Free Header */
				/* 4. Coalece if necessary */
				while((reqBlkSize > freelist_head->header.block_size) && pageCount < 4){
					hptr = ics_inc_brk();
					setEpilogue(&epilogue);
					pageCount++;

					// Case 1 : Free block is NOT at the end of the previous page
					if(((char *)freelist_head + freelist_head->header.block_size + WSIZE) != hptr)
					{
						/* Create a new node, insert it into the FL at the head. */
						ics_free_header *newFree = hptr;
						setFreeHeader(newFree, NULL, freelist_head, PSIZE, 0);
						freelist_head->prev = newFree;
						freelist_head = newFree;
					}
					// Case 2: Free block is at end of the previous page. Coalesce.
					else
					{
						/* Change the header of the existing node, create a footer then insert into FL */
						freelist_head->header.block_size += PSIZE;
						setFooter(&(freelist_head->header));
					}
				}
				if((reqBlkSize > freelist_head->header.block_size) && pageCount == 4){
					errno = ENOMEM;
					return NULL;
				}

				allocMem(&hptr, &freelist_head, &freelist_head, size, &padByte);
				return hptr;

			}
		}
		// Change the header, create a new free block, insert it into the FL. 
		else
		{
			// If the memory space availabe after the allocation is going to be < 32, add those bytes to padding. 
			allocMem(&hptr, &freelist_head, &freelist_head, size, &padByte);
			return hptr;
		}
	}
	// Free List consists 2 or more nodes. BEST FIT
	else
	{
		ics_free_header *currentBest = freelist_head;
		while(freelist_head != NULL)
		{
			// Free Block list does not contain a block that can satisfy allocation. Request for a page of memory.
			if((freelist_head->header.block_size < reqBlkSize) && (currentBest == NULL))
			{
				while((reqBlkSize > freelist_head->header.block_size) && pageCount < 4){
					hptr = ics_inc_brk();
					setEpilogue(&epilogue);
					pageCount++;

					// Case 1 : Free block is NOT at the end of the previous page
					if(((char *)freelist_head + freelist_head->header.block_size + WSIZE) != hptr)
					{
						/* Create a new node, insert it into the FL at the head. */
						ics_free_header *newFree = hptr;
						setFreeHeader(newFree, NULL, freelist_head, PSIZE, 0);
						freelist_head->prev = newFree;
						freelist_head = newFree;
					}
					// Case 2: Free block is at end of the previous page. Coalesce.
					else
					{
						/* Change the header of the existing node, create a footer then insert into FL */
						freelist_head->header.block_size += PSIZE;
						setFooter(&(freelist_head->header));
					}
				}
				if((reqBlkSize > freelist_head->header.block_size) && pageCount == 4){
					errno = ENOMEM;
					return NULL;
				}
				allocMem(&hptr, &freelist_head, &freelist_head, size, &padByte);
				return hptr;
			}
			if((freelist_head->header.block_size >= reqBlkSize) 
				&& (currentBest->header.block_size >= freelist_head->header.block_size))
			{
				currentBest = freelist_head;
			}
			if(freelist_head->header.block_size < reqBlkSize)
			{
				if(holdHead == currentBest){
					freelist_head = holdHead->next;
				}
				else{
					freelist_head = holdHead;
				}
				removeNode(&holdHead, currentBest);
				allocMem(&hptr, &freelist_head, &currentBest, size, &padByte);
				return hptr;
			}
			freelist_head = freelist_head->next;
		}
		if(holdHead == currentBest){
			freelist_head = holdHead->next;
		}
		else{
			freelist_head = holdHead;
		}
		removeNode(&holdHead, currentBest);
		allocMem(&hptr, &freelist_head, &currentBest, size, &padByte);
		return hptr;

	}
	
    return NULL; 
}

void *ics_realloc(void *ptr, size_t size) {
	uint64_t padByte = 0;
	if(size == 0)
	{
		ics_free(ptr);
		return NULL;
	}

	ics_header *tempHeader = ptr - WSIZE;
	if(size % DSIZE != 0)
	{
		padByte = DSIZE - (size % DSIZE);
	}
	size_t reqBlkSize = size + padByte + DSIZE;

	// memcpy(copy, ptr, newPayload);
	

	// No need to move, minimize space, adjust padding_amount, adjust footer address prevent splitting.
	if(reqBlkSize <= (tempHeader->block_size & ~3))
	{
		tempHeader->block_size = tempHeader->block_size & ~2;
		// Add more padding to prevent splinter, return without chaning header and footer.
		if((tempHeader->block_size - reqBlkSize) < 32)
		{
			tempHeader->padding_amount = padByte + 16;
			return ptr;
		}
		// Add Remaining free space as a block to the Free List
		else
		{
			tempHeader->block_size = reqBlkSize + 1;
			if(padByte != 0)
				tempHeader->block_size += 2;
			tempHeader->hid = HID;
			tempHeader->padding_amount = padByte;
			setFooter(tempHeader);

			ics_free_header *newNode = (void *)tempHeader + tempHeader->block_size;
			newNode->next = NULL;
			newNode->prev = NULL;
			newNode->header.hid = HID;
			newNode->header.block_size = tempHeader->block_size - reqBlkSize;
			newNode->header.padding_amount = 0;
			setFooter(&(newNode->header));
			insertInOrder(&freelist_head, &newNode);
			return ptr;
		}
	}
	// Relocate, copy the content then free the previous space.
	else
	{
		void *newBlk = ics_malloc(size);
		memcpy(newBlk, ptr, tempHeader->block_size);
		ics_free(ptr);
		return newBlk;
	}

    return NULL; 
}

int ics_free(void *ptr) {
	ics_header *tempHeader = ptr - 8;
	ics_footer *tempFooter = ptr + (tempHeader->block_size & ~3) - DSIZE;
	
	/* 1) ptr is in between the prologue and epilogue */
	if((ptr < (void *)prologue) || (ptr > (void *)epilogue)){
		errno = EINVAL;
		return -1;
	}
	/* 2) The allocated bit 'a' is set in both the block's header and footer */
	if(((tempHeader->block_size & 1) != 1) || ((tempFooter->block_size & 1) != 1)){
		errno = EINVAL;
		return -1;
	}
	/* 3) Check the hid of the block's header for macro: HID */
	/* 4) Check the fid of the block's footer for macro: FID */
	if((tempHeader->hid != HID) || (tempFooter->fid != FID)){
		errno = EINVAL;
		return -1;
	}
	/* 5) If the header/footer padding bit 'p' is set, the padding_size is greater than 0 */
	if(((tempHeader->block_size & 2) == 2) && tempHeader->padding_amount == 0){
		errno = EINVAL;
		return -1;
	}
	/* 6) Check that block_size in the block's header and footer are equal */
	if(tempHeader->block_size != tempFooter->block_size){
		errno = EINVAL;
		return -1;
	}

	/* Free allocated block and insert it in to the Free List */
	/* 1) Set the allocated bit to 0 */
	tempHeader->block_size = tempHeader->block_size & ~3;
	tempHeader->padding_amount = 0;
	tempFooter->block_size = tempHeader->block_size;
	ics_free_header *newFreeBlk = NULL;
	// Both sides of the freeing block is currently allocated. Case 1
	/*
	if(((((ics_footer *)(ptr-DSIZE))->block_size & 1) == 1) && 
		(((ics_header *)(ptr + (tempHeader->block_size & ~3) - WSIZE))->block_size & 1) == 1)
	{
		newFreeBlk = (ics_free_header *)tempHeader;
		insertInOrder(&freelist_head, &newFreeBlk);
		return 0;
	}
	*/
	// Previous block is free, coalesce. Case 3
	// else if(((((ics_footer *)(ptr-DSIZE))->block_size & 1) == 0) && 
	// 	(((ics_header *)(ptr + tempHeader->block_size - WSIZE))->block_size & 1) == 1)
	if((((ics_footer *)(ptr-DSIZE))->block_size & 1) == 0)
	{
		tempFooter = ptr - DSIZE;
		newFreeBlk = (void *)tempHeader - tempFooter->block_size;
		newFreeBlk->header.block_size += tempHeader->block_size;
		newFreeBlk->header.padding_amount = 0;
		newFreeBlk->header.hid = HID;
		tempFooter = (void *)newFreeBlk + newFreeBlk->header.block_size - WSIZE;
		tempFooter->block_size = newFreeBlk->header.block_size;
		tempFooter->fid = FID;
		removeNode(&freelist_head, newFreeBlk);
		insertInOrder(&freelist_head, &newFreeBlk);
		return 0;
	}
	
	// Next block is free, coalesce. Case 2 (Extra Credit)
	/*
	else if(((((ics_footer *)(ptr-DSIZE))->block_size & 1) == 1) && 
		(((ics_header *)(ptr + tempHeader->block_size - WSIZE))->block_size & 1) == 0)
	{
		ptr += tempHeader->block_size - WSIZE;
		ics_free_header *temp = (ics_free_header *)ptr;
		newFreeBlk = (ics_free_header *)tempHeader;
		newFreeBlk->header.block_size += ((ics_header *)ptr)->block_size;
		newFreeBlk->header.padding_amount = 0;
		newFreeBlk->header.hid = HID;
		tempFooter = (void *)tempFooter + ((ics_header *)ptr)->block_size;
		tempFooter->block_size = newFreeBlk->header.block_size;
		tempFooter->fid = FID;
		removeNode(&freelist_head, temp);
		insertInOrder(&freelist_head, &newFreeBlk);
	}
	*/
	
	/*
	// Blocks on both sides are free, coalesce both sides. Case 4 (Extra Credit)
	else
	{
		tempFooter = ptr - DSIZE;
		newFreeBlk = (void *)tempHeader - tempFooter->block_size;
		newFreeBlk->header.block_size += tempHeader->block_size;
		tempHeader = (void *)tempFooter + WSIZE;
		tempFooter = (void *)tempFooter + newFreeBlk->header.block_size + tempHeader->block_size;
		newFreeBlk->header.block_size += tempFooter->block_size;
		tempFooter->block_size = newFreeBlk->header.block_size;
		tempFooter->fid = FID;
		insertInOrder(&freelist_head, &newFreeBlk);
	}
	*/
	// Both sides of the freeing block is currently allocated. Case 1
	newFreeBlk = (ics_free_header *)tempHeader;
	insertInOrder(&freelist_head, &newFreeBlk);
	
    return 0; 
}


