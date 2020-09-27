#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

//Змінна для обробки помилок
extern int errno;

//int hasInitialized = 0;
void *managedMemoryStart;
void *lastValidAddress;

int totalAllocatedSpace;
int totalFreeSpace;

#pragma pack(push, 4)
struct blocHeader {
	struct blocHeader *prev;
	unsigned int isAvailable:1;
	unsigned int size:31;
};
#pragma pack(pop)

const size_t sizeOfHeader = sizeof(struct blocHeader);
//////////MAYBE DELETE///////////////
// void mem_alloc_init () {
// 	lastValidAddress = sbrk(0);
// 	managedMemoryStart = lastValidAddress;
// 	hasInitialized = 1;
// }
/////////////////////////////////////
void *mem_alloc (int numbytes) {
	//Ініціалізація алокатора
	static short int hasInitialized = 0;
	if (!hasInitialized) {
		lastValidAddress = sbrk(0);
		managedMemoryStart = lastValidAddress;
		hasInitialized = 1;
	}

	void *currentLocation;
	struct blocHeader *currentHeader;
	void *memoryLocation = 0; //return value
	static struct blocHeader *lastAllocated = NULL;

	numbytes += sizeof(struct blocHeader);
	currentLocation = managedMemoryStart;
	while (currentLocation != lastValidAddress) {
		currentHeader = (struct blocHeader*)currentLocation;
	 	if (currentHeader->isAvailable == 0) { 		//flag isAvailable
	 		if (currentHeader->size >= numbytes) {
	 			struct blocHeader *next = (struct blocHeader*)(currentLocation + numbytes);
	 			next->isAvailable = 0;
	 			next->size = currentHeader->size - numbytes;
	 			next->prev = currentHeader;
	 			currentHeader->isAvailable = 1;
	 			currentHeader->size = numbytes;
	 			memoryLocation = currentLocation;
	 			break;
	 		}
	 	}
	 	currentLocation += currentHeader->size;
	 }
	 if (!memoryLocation) {
	 	sbrk(numbytes);
	 	if (errno) return NULL;
	 	memoryLocation = lastValidAddress;
	 	lastValidAddress += numbytes;
	 	currentHeader = memoryLocation;
	 	currentHeader->size = numbytes;
	 	currentHeader->isAvailable = 1;
	 	currentHeader->prev = lastAllocated;
	 }
	 currentHeader->prev = lastAllocated;
	 lastAllocated = memoryLocation;
	 memoryLocation += sizeof(struct blocHeader);
	 return memoryLocation;
}

void mem_free (void *bloc) {
	struct blocHeader *currentHeader;
	currentHeader = bloc - sizeof(struct blocHeader);
	//currentHeader->size = abs(currentHeader->size);
	currentHeader->isAvailable = 0;

	struct blocHeader *next;
	next = (struct blocHeader*)((void*)currentHeader + currentHeader->size);
	if (next->isAvailable == 0) {
		currentHeader->size += next->size;
	}
	if (currentHeader->prev->isAvailable == 0) {
		currentHeader->prev->size += currentHeader->size;
	}
}

void *mem_realloc (void *bloc, int size) {
	if (bloc == NULL) {
		return mem_alloc(size);
	}
	struct blocHeader *header;
	header = (struct blocHeader*)(bloc - sizeof(struct blocHeader));
	int diff = size - header->size;
	if ((void*)header + header->size == lastValidAddress) {
		sbrk(diff);
		header->size += diff;
		return bloc;
	}
	void *dest = mem_alloc(size);
	if (diff > 0) memcpy(bloc, dest, header->size - sizeOfHeader);
	else memcpy(bloc, dest, size);
	mem_free(bloc);
	return dest;
}

void mem_dump () {
	void *currentLocation = managedMemoryStart;
	struct blocHeader *currentHeader;
	int counter = 1;
	printf("\nManaged memory starts at %X\n", managedMemoryStart);
	while (currentLocation != lastValidAddress) {
		currentHeader = (struct blocHeader*)currentLocation;
		printf("%d\tstart: %X\tend: %X\tsize: %d\toccupation: %d\n",
			counter,
			currentLocation,
			currentLocation + currentHeader->size,
			currentHeader->size - sizeOfHeader,
			currentHeader->isAvailable);

		currentLocation += currentHeader->size;
		counter++;
	}
	printf("Managed memory ends at %X\n", lastValidAddress);
}

int main () {
	void *first = mem_alloc(100);
	void *second = mem_alloc(200);
	void *third = mem_alloc(100);
	void *fourth = mem_alloc(200);
	void *fifth = mem_alloc(100);
	mem_dump();

	mem_free(fourth);
	mem_free(second);
	mem_dump();
	
	mem_free(third);	
	void *sixth = mem_alloc(100000000);
	second = mem_alloc(248);
	mem_dump();
	
	/*
	for (int i = 1; ; i++) {
		void *mem = mem_alloc(100*i);
		if (mem == NULL || i == 10000) break;
	}
	mem_dump();
	*/
	return 0;
}