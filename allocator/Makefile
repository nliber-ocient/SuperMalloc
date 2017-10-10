ROOT      = ..
BUILD     = allocator
OPTFLAGS  = -O3
#UNITTESTS = unit-tests
#TESTING   = -DTESTING
LINKMODE  = static
PREFIX    = supermalloc_
include ../Makefile.include

supermalloc_allocator.h : supermalloc.h

main : main.cpp supermalloc_allocator.h $(LIB)/libsupermalloc.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fexceptions $(LDFLAGS) -lsupermalloc -o $(@) $(<)

