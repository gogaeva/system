void *poolAlloc(Pool *buff) {
	if (!buff)
		return NULL;

	if (!buff->head) 
		return NULL;

	uint8_t *currPtr = buff->head;
	buff->head = (*((uint8_t**)(buff->head)));
	return currPtr;
}

void poolFree (Pool *buff, void *ptr) {
	if (!buff || !ptr)
		return;

	*((uint8_t**)ptr) = buff->head;
	buff->head = (uint8_t*)ptr;
	return;
}

void poolInit (Pool *buff, uint8_t* mem, uint32_t size, uint32_t chunkSize) {
	if (!buff || !mem || !size || !chunkSize)
		return;

	const uint32_t chunkCount = (size / chunkSize) - 1;
	for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
		uint8_t *currChunk = mem + (chunkIndex * chunkSize);
		*((uint8_t**)currChunk) = currChunk + chunkSize;
	}
	*((uint8_t**)&mem[chunkCount * chunkSize]) = NULL;
	buff->mem = buff->head = mem; 
	return;
}