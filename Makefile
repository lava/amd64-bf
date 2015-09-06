.PHONY: clean

CFLAGS = -g -O0

run: hello.elf ld-bf.so
	./hello.elf

start.o: start.S
	$(CC) $(CFLAGS) $^ -c -o $@

interp.o: interp.c
	$(CC) $(CFLAGS) -std=gnu11 -nostdlib -fPIC -m64 $^ -c -o $@

# The dynamic loader *must* be relocatable, or it can/will
# overwrite the interpretee data with its own memory image
ld-bf.so: start.o interp.o
	$(LD) $(LDFLAGS) -nostdlib -shared $^ -o $@

bold: bold.c
	$(CC) $(CFLAGS) $^ -o $@

hello.elf: bold hello.bf
	./bold hello.bf $@

clean::
	rm -f hello.elf bold ld-bf.so interp.o start.o
