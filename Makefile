VERSION:=$(shell git rev-parse HEAD)
STAGE_PATH ?= stage

.PHONY: allocator
allocator:
	(cd allocator && make)

install: allocator
	mkdir -p $(STAGE_PATH)/lib/ $(STAGE_PATH)/include/
	cp allocator/lib/libsupermalloc.so allocator/lib/libsupermalloc_pthread.so $(STAGE_PATH)/lib/
	cp allocator/supermalloc.h allocator/supermalloc_allocator.h $(STAGE_PATH)/include/

.PHONY: clean
clean:
	(cd allocator && make clean)
	rm -f libsupermalloc-*.tar.gz

archive: install
	git diff --quiet	# fails if there are local unstaged changes
	git archive --prefix=libsupermalloc-$(VERSION)/ -o libsupermalloc-$(VERSION).tar.gz $(VERSION)

.PHONY: release
release:
	cd release; make other-mallocs; make

