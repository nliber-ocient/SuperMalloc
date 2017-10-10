CXXFLAGS = -W -Wall -Werror -O0 -ggdb3 -std=c++17
supermalloc.pdf: test-malloc_test-1K-aggregated.pdf

%.pdf: %.gnuplot %.data
	gnuplot $<
test-malloc_test-1K-aggregated.data: aggregate_error_bars
	./$< < ../tests/test-malloc_test-1K.data > $@
