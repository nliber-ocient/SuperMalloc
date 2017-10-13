VERSION:=$(shell git rev-parse HEAD)

.PHONY: allocator
	(cd allocator && make)

.PHONY: clean
clean:
	(cd allocator && make clean)
	rm -f supermalloc-*.tar.gz

.PHONY: archive
archive:
	git diff --quiet	# fails if there are local unstaged changes
	git archive --prefix=supermalloc-$(VERSION)/ -o supermalloc-$(VERSION).tar.gz $(VERSION)

.PHONY: release
release:
	cd release; make other-mallocs; make

