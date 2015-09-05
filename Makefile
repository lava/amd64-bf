.PHONY: clean

CXX = gcc
CXXFLAGS = -std=gnu11

run: hello.elf ld-bf.so
	./hello.elf

# The dynamic loader *must* be relocatable, or it will/can
# overwrite the interpretee data with its own memory image
ld-bf.so: start.S interp.c
	$(CXX) $(CXXFLAGS) -fPIC -shared -nostdlib -m64 -ggdb $^ -o $@

bold: bold.c
	$(CXX) $^ -o $@

hello.elf: bold hello.bf
	./bold hello.bf $@

clean::
	rm -f hello.elf bold ld-bf.so interp.o start.o
