typedef uint32_t StackHandle;

void *stackBufferAlloc (StackBuffer *buf, uint32_t size, StackHandle *handle) {
	if (!buf || !size) 
		return NULL;

	const uint32_t currOffset = buf->offset;
	if (currOffset + size <= buf->totalSize) {
		uint8_t *ptr = buf->mem + currOffset;
		buf->offset += size;

		if(handle)
			*handle = currOffset;

		return (void*)ptr;			
	}
	return NULL;
}

void stackBufferSet (StackBuffer *buf, StackHandle handle) {
	buf->offset = handle;
	return;
}