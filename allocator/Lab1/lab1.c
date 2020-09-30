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

//#pragma pack(push, 4)
struct blocHeader {
	//struct blocHeader *prev;
	unsigned int isOccupied:1;
	unsigned int size:31;
	int prevSize;
};
//#pragma pack(pop)

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
	static struct blocHeader *lastBloc = NULL;
	if (!hasInitialized) {
		lastValidAddress = sbrk(0);
		managedMemoryStart = lastValidAddress;
		hasInitialized = 1;
	}

	void *currentLocation = managedMemoryStart;
	struct blocHeader *current = (struct blocHeader*)currentLocation;
	void *memoryLocation = NULL; //return value

	numbytes += sizeOfHeader;
	//currentLocation = managedMemoryStart;
	while (currentLocation != lastValidAddress) {
		current = (struct blocHeader*)currentLocation;
	 	if (current->isOccupied == 0 && current->size >= numbytes) { 		
	 		struct blocHeader *rest = (struct blocHeader*)(currentLocation + numbytes);
	 		rest->isOccupied = 0;
	 		rest->size = current->size - numbytes;
	 		rest->prevSize = numbytes;
	 		struct blocHeader *next = (struct blocHeader*)(currentLocation + current->size);
	 		//враховуємо, що наш блок може бути останнім, і після нього ніякого next немає
	 		if (next != lastValidAddress) {
	 			if (next->isOccupied = 1)
	 				next->prevSize = rest->size;
	 			else
	 				rest->size += next->size;
	 		}
	 		current->isOccupied = 1;
	 		current->size = numbytes;
	 		memoryLocation = currentLocation;
	 		break;
	 	}
	 	currentLocation += current->size;
	 }
	//Якщо серед наявних блоків немає такого, що підходить, 
	//алокуємо його шляхом розширення буфера: 
	if (!memoryLocation) {
		//1) Новий блок буде алокований у кінці буфера
	 	if (lastBloc) {
	 		if (current->isOccupied == 1){
	 			if ((int)sbrk(numbytes) == -1)
	 				return NULL;
	 			current = (struct blocHeader*)lastValidAddress;
	 			lastValidAddress += numbytes;
	 			current->isOccupied = 1;
	 			current->size = numbytes;
	 			current->prevSize = lastBloc->size;
	 			lastBloc = current;
	 		}
	 		//2) Якщо останній блок у буфері вільний але замалий, 
	 		//розширемо його і повернемо вказівник на нього
	 		else {
				int diff = numbytes - current->size;
	 			if ((int)sbrk(diff) == -1)
	 				return NULL;
	 			lastValidAddress += diff;
	 			current->isOccupied = 1;
	 			current->size += diff;
			}
			memoryLocation = (void*)current;
		}
		//3) Це перший виклик алокатора і новий блок буде знаходитись на початку буфера
		else {
			if ((int)sbrk(numbytes) == -1)
				return NULL;
			memoryLocation = managedMemoryStart;
			lastValidAddress += numbytes;
			current->isOccupied = 1;
			current->size = numbytes;
			current->prevSize = 0;
			lastBloc = current;
		}
		memoryLocation += sizeOfHeader;
	 	return memoryLocation;
	}
}

void mem_free (void *bloc) {
	struct blocHeader *current;
	current = bloc - sizeOfHeader;
	//currentHeader->size = abs(currentHeader->size);
	//current->isOccupied = 0;
	if (current->isOccupied){
		struct blocHeader *next, *prev, *afterNext;
	//struct blocHeader *prev;
		prev = (struct blocHeader*)((void*)current - current->prevSize);
		next = (struct blocHeader*)((void*)current + current->size);
		afterNext = NULL;
		if ((void*)next != lastValidAddress && next->isOccupied == 0) {
			afterNext = (struct blocHeader*)((void*)next + next->size);
			current->size += next->size;
			if ((void*)afterNext != lastValidAddress)
				afterNext->prevSize = current->size;
		}
		if (prev->isOccupied == 0) {
			//current->prev->size += currentHeader->size;
			prev->size += current->size;
			if (afterNext) 
				afterNext->prevSize = prev->size;
			else
				next->prevSize = prev->size;
		}
		current->isOccupied = 0;
	}
}

void *mem_realloc (void *bloc, int newsize) {
	if (bloc == NULL) {
		return mem_alloc(newsize);
	}
	newsize += sizeOfHeader;
	struct blocHeader *header;
	header = (struct blocHeader*)(bloc - sizeOfHeader);
	int diff = newsize - header->size;
	if (diff < 0) {
		struct blocHeader *rest, *next;
		rest = (struct blocHeader*)((void*)header + newsize);
		next = (struct blocHeader*)((void*)header + header->size);
		rest->isOccupied = 0;
		rest->prevSize = newsize;
		if (next != lastValidAddress && next->isOccupied == 1) {
			rest->size = -diff;
			next->prevSize = rest->size;
		}
		else {
			rest->size = -diff + next->size;	
		}
		header->size = newsize;
		return bloc;
	}
	if ((void*)header + header->size == lastValidAddress) {
		if ((int)sbrk(diff) == -1)
			return NULL;
		//sbrk(diff);
		header->size += diff;
		return bloc;
	}
	
	newsize -= sizeOfHeader;
	void *dest = mem_alloc(newsize);
	memcpy(bloc, dest, header->size - sizeOfHeader);
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
			currentHeader->isOccupied);

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

	sixth = mem_realloc(sixth, 50000000);
	mem_dump();

	void *seventh = mem_alloc(40000000);
	seventh = mem_realloc(seventh, 60000000);
	void *eighth = mem_realloc(first, 350);
	fifth = mem_realloc(fifth, 50);
	mem_dump();

	return 0;
}