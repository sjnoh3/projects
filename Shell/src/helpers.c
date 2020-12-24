#include "helpers.h"

/* Helper function definitions go here */
void setEpilogue(ics_header **epilogue){
	(*epilogue) = ics_get_brk() - 8;
	(*epilogue)->block_size = 1;
	(*epilogue)->padding_amount = 0;
	(*epilogue)->hid = HID;
}

void setFreeHeader(ics_free_header *newFree, ics_free_header *prev, ics_free_header *next, uint64_t block_size, uint64_t padding_amount){
	newFree->prev = prev;
	newFree->next = next;
	newFree->header.block_size = block_size;
	newFree->header.hid = HID;
	newFree->header.padding_amount = padding_amount;
}

void setFooter(ics_header *header){
	ics_footer *footer = (void *)header + (header->block_size & ~3) - 8;
	footer->block_size = header->block_size;
	footer->fid = FID;
}

void allocMem(void **hptr, ics_free_header **freelist_head, ics_free_header **freeBlk, size_t size, uint64_t *padByte){
	int allocBlkSize;
	if((*freeBlk)->header.block_size - (size + *padByte + DSIZE) == 16)
	{
		*padByte += DSIZE;	
	}
	else if((*freeBlk)->header.block_size - (size + *padByte + DSIZE) >= 16)
	{
		ics_free_header *newNode = (void *)(*freeBlk)+(size + *padByte + DSIZE);
		newNode->header.block_size = (*freeBlk)->header.block_size - (size + *padByte + DSIZE);
		newNode->header.hid = HID;
		newNode->header.padding_amount = 0;
		newNode->next = NULL;
		newNode->prev = NULL;
		insertInOrder(freelist_head, &newNode);
	}
	int freeBlkSize = (*freeBlk)->header.block_size;
	(*freeBlk)->header.block_size = size + *padByte + DSIZE + 1;
	if(*padByte != 0)
	{
		(*freeBlk)->header.block_size += 2;
		(*freeBlk)->header.padding_amount = *padByte;
	}

	*hptr = (*freeBlk);
	setFooter(&((*freeBlk)->header));
	allocBlkSize = (*freeBlk)->header.block_size & ~3;
	(*freeBlk) = *hptr + allocBlkSize;
	if((freeBlkSize - allocBlkSize) == 0)
	{
		*freeBlk = NULL;
	}
	else
	{
		setFreeHeader((*freeBlk), NULL, NULL, freeBlkSize - allocBlkSize, 0);
		setFooter(&((*freeBlk)->header));	
	}
	*hptr += WSIZE;


	return;
}

void insertInOrder(ics_free_header **freelist_head, ics_free_header **newNode){
	ics_free_header *temp = NULL;
	while((*freelist_head) != NULL)
	{
		if((*newNode)->header.block_size >= (*freelist_head)->header.block_size)
		{
			temp = (*freelist_head)->prev;
			(*newNode)->next = *freelist_head;
			(*newNode)->prev = temp;
			(*freelist_head)->prev = *newNode;
			if(temp == NULL){
				(*freelist_head) = *newNode;
				return;
			}
			temp->next = *newNode;
			while((*freelist_head)->prev != NULL)
			{
				(*freelist_head) = (*freelist_head)->prev;
			}
			return;
		}
		temp = *freelist_head;
		(*freelist_head) = (*freelist_head)->next;
	}
	(*newNode)->next = NULL;
	(*newNode)->prev = temp;
	if(temp == NULL)
	{
		(*freelist_head) = (*newNode);
		return;
	}
	temp->next = *newNode;	
	(*freelist_head) = temp;
	while((*freelist_head)->prev != NULL)
	{
		(*freelist_head) = (*freelist_head)->prev;
	}		
	return;
}

void removeNode(ics_free_header **freelist_head, ics_free_header *temp){
	ics_free_header *prevFree = temp->prev;
	ics_free_header *nextFree = temp->next;
	if((prevFree != NULL) && (nextFree != NULL))
	{
		prevFree->next = nextFree;
		nextFree->prev = prevFree;
	}
	else if((prevFree != NULL) && (nextFree == NULL))
	{
		prevFree->next = NULL;
	}	
	else if((prevFree == NULL) && (nextFree != NULL))
	{
		nextFree->prev = NULL;
		(*freelist_head) = nextFree;
	}
	else
	{
		(*freelist_head) = NULL;
	}
	return;
}



