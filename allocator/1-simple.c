//System break - "край" размеченной памяти
//sbrk(n) - отодвигает Current break на n байт и возвращает указатель на него

#include <unistd.h> // sbrk function

int hasInitialized = 0;
void *managedMemoryStart;
void *lastValidAddress;

void mallocInit () {
	lastValidAddress = sbrk(0); // system break
	/* пока у нас нет памяти, которой можно было бы управлять
	 * установим начальный указатель на last_valid_address
	 */
	managedMemoryStart = lastValidAddress;
	//Инициализация прошла, теперь можно пользоваться 
	hasInitialized = 1; 
}

struct memControlBloc {
	int isAvailable;
	int size;
};

void free (void *firstByte) {
	struct memControlBloc *mcb;
	mcb = firstByte - sizeof(struct memControlBloc);
	mcb -> isAvailable = 1;
	return;
}

void *malloc (long numbytes) {
	void *currentLocation; 
	struct memControlBloc currentLocation_mcb;
	void *memoryLocation;

	if (!hasInitialized) {
		mallocInit();
	}

	numbytes += sizeof(struct memControlBloc);
	memoryLocation = 0;
	currentLocation = managedMemoryStart;
	while (currentLocation != lastValidAddress) {
		currentLocation_mcb = (struct memControlBloc *)currentLocation;
		if (currentLocation_mcb -> isAvailable) {
			if (currentLocation_mcb -> size >= numbytes){
				currentLocation_mcb -> isAvailable = 0;
				memoryLocation = currentLocation;
				break;
			}
		}
		currentLocation += currentLocation_mcb->size;
	}
	if (!memoryLocation) {
		sbrk(numbytes);
		memoryLocation = lastValidAddress;
		lastValidAddress += numbytes;
		currentLocation_mcb = memoryLocation;
		currentLocation_mcb -> isAvailable = 0;
		currentLocation_mcb -> size = numbytes;
	}
	memoryLocation += sizeof(struct memControlBloc);
	return memoryLocation;
}
