char *malloc (size_t size) {
	if (!size) {
		return 0;
	}
	char *addr = 0;
	int i;
	for (i = 0; i < allocationAvlTree.size; ) {
		int r;
		node_t *n;
		n = &allocationAvlTree.nodes[i];
		if (!n->key) {
			return NULL;
		}
		r = allocationAvlTree.cmp(n->key, size);
		if (r <= 0) {
			
		}
	}
}