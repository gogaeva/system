typedef struct _LinearBuffer {
	uint8_t *mem;
	uint32_t totalSize;
	uint32_t offset;
} LinearBuffer;

void *linearBufferAlloc (LinearBuffer *buf, uint32_t size) {
	if (!buf || !size) return NULL;

	uint32_t newOffset = buf->offset + size;
	if (newOffset <= buf->totalSize) {
		void *ptr = buf->mem + buf->offset;
		buf->offset = newOffset;
		return ptr;
	}
	return NULL;
}