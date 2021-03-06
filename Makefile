.PHONY: clean distclean deb

CFLAGS = -g -O2

LDPATH = $(PREFIX)/lib64/ld-bf.so.0

all: bold ld-bf.so

run: hello.elf ld-bf.so
	./hello.elf

start.o: start.S
	$(CC) $(CFLAGS) $^ -c -o $@

interp.o: interp.c
	$(CC) $(CFLAGS) -std=gnu11 -nostdlib -fPIC -m64 $^ -c -o $@

# The dynamic loader *must* be relocatable, or it can/will
# overwrite the interpretee data with its own memory image
ld-bf.so: start.o interp.o
	$(LD) $(LDFLAGS) -nostdlib -shared $^ -o $@ -soname=ld-bf.so.0

bold: bold.c
	$(CC) $(CFLAGS) $^ -o $@ -DLDPATH=$(LDPATH)

hello.elf: bold hello.bf
	./bold hello.bf $@

install:: ld-bf.so
	mkdir -p $(dir $(DESTDIR)/$(LDPATH))
	cp $^ $(DESTDIR)/$(LDPATH)

install:: bold
	mkdir -p $(DESTDIR)/$(PREFIX)/usr/bin
	cp $^ $(DESTDIR)/$(PREFIX)/usr/bin/$^

deb: 
	debuild 

clean::
	rm -f interp.o start.o

distclean:: clean
	rm -f hello.elf bold ld-bf.so
