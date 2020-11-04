#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *managedMemoryStart;
void *lastValidAddress;

struct blocHeader {
	unsigned int isOccupied:1;
	unsigned int size:31;
	int prevSize;
};

const size_t sizeOfHeader = sizeof(struct blocHeader);
struct blocHeader *lastBloc = NULL;

void *mem_alloc (int numbytes) {
	//Ініціалізація алокатора
	static short int hasInitialized = 0;
	//Цей блок виконується тільки після першого виклику mem_alloc
	if (!hasInitialized) {
		lastValidAddress = sbrk(0);
		managedMemoryStart = lastValidAddress;
		hasInitialized = 1;
	}

	void *currentLocation = managedMemoryStart;
	struct blocHeader *current = (struct blocHeader*)currentLocation;
	void *memoryLocation = NULL; //return value

	numbytes += sizeOfHeader;
	//Наявні блоки пам'яті перебираються у пошуках вільного
	while (currentLocation != lastValidAddress) {
		current = (struct blocHeader*)currentLocation;
	 	if (!current->isOccupied && current->size >= numbytes) {
	 		//забираємо з вільного блоку потрібну кількість байт, а з того, що залишилось,
	 		//робимо новий вільний блок  		
	 		struct blocHeader *rest = (struct blocHeader*)(currentLocation + numbytes);
	 		rest->isOccupied = 0;
	 		rest->size = current->size - numbytes;
	 		rest->prevSize = numbytes;
	 		//перевіряємо наступний блок: якщо він вільний - склеюємо його з залишком
	 		//якщо ні, то вносимо зміни в його заголовок (поле prevSize)
	 		struct blocHeader *next = (struct blocHeader*)(currentLocation + current->size);
	 		//враховуємо, що наш блок може бути останнім, і після нього немає наступного
	 		if (next != lastValidAddress) {
	 			if (next->isOccupied)
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
	 		if (current->isOccupied){/////////////////////
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
	 		//розширимо його і повернемо вказівник на нього
	 		else {
	 			numbytes -= sizeOfHeader;
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
	}
	memoryLocation += sizeOfHeader;
	return memoryLocation;
}

void mem_free (void *bloc) {
	struct blocHeader *current;
	current = bloc - sizeOfHeader;
	if (current->isOccupied){
		struct blocHeader *next, *prev, *afterNext;
		prev = (struct blocHeader*)((void*)current - current->prevSize);
		next = (struct blocHeader*)((void*)current + current->size);
		afterNext = NULL;
		//якщо один або обидва сусідні блоки вільні, блок, що звільняється, склеюється з ними
		if ((void*)next != lastValidAddress && !next->isOccupied) {
			afterNext = (struct blocHeader*)((void*)next + next->size);
			current->size += next->size;
			if ((void*)afterNext != lastValidAddress)
				afterNext->prevSize = current->size;
		}
		if (!prev->isOccupied) {
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
	struct blocHeader *rest, *next;
	if (diff < 0) {
		rest = (struct blocHeader*)((void*)header + newsize);
		next = (struct blocHeader*)((void*)header + header->size);
		rest->isOccupied = 0;
		rest->prevSize = newsize;
		//ті ж перевірки для склеювання сусідніх вільних блоків
		if (next != lastValidAddress && next->isOccupied) {
			rest->size = -diff;
			next->prevSize = rest->size;
		}
		else if (next != lastValidAddress && !next->isOccupied) {
			rest->size = -diff + next->size;	
		}
		else {
			rest->size = -diff + next->size;
			lastBloc = rest;
		}
		header->size = newsize;
		return bloc;
	}

	next = (struct blocHeader*)((void*)header + header->size);
	//якщо наступний блок вільний і має достатній розмір для розширення даного, беремо шматок від нього
	if (!next->isOccupied && next->size >= diff) {
		rest = (struct blocHeader*)((void*)next + diff);
		header->size += diff;
		rest->isOccupied = 0;
		rest->size = next->size - diff;
		rest->prevSize = header->size;
		return bloc;
	}
	//якщо за даним блоком йде вільний блок недостатньго розміру для розширення і він останній,
	//приклеюємо цей блок до даного і пересуваємо break address
	if (!next->isOccupied && (void*)next + next->size == lastValidAddress) {
		diff -= next->size;
		if ((int)sbrk(diff) == -1)
			return NULL;
		header->size += (next->size + diff);
		lastValidAddress += diff;
		return bloc;
	}
	//якщо треба розширити останній блок, то просто пересуваємо break address
	if ((void*)header + header->size == lastValidAddress) {
		if ((int)sbrk(diff) == -1)
			return NULL;
		header->size += diff;
		lastValidAddress += diff;
		return bloc;
	}
	//в усіх інших випадках шукаємо новий блок і переписуємо дані
	void *dest = mem_alloc(newsize - sizeOfHeader);
	memcpy(dest, bloc, header->size - sizeOfHeader);
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
	int *first = (int*)mem_alloc(25 * sizeof(int));
	int i = 0;
	while (i < 25) {
		first[i] = i;
		i++;
	}
	void *second = mem_alloc(200);
	void *third = mem_alloc(100);
	void *fourth = mem_alloc(200);
	void *fifth = mem_alloc(100);
	mem_dump();
	printf("arr[5] = %d\n", first[5]);
	printf("arr[20] = %d\n", first[20]);

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
	int *eighth = (int*)mem_realloc(first, 90 * sizeof(int));
	fifth = mem_realloc(fifth, 50);
	fifth = mem_realloc(fifth, 58);
	mem_dump();
	printf("arr[5] = %d\n", eighth[5]);
	printf("arr[20] = %d\n", eighth[20]);
	return 0;
}