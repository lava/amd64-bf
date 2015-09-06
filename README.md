# amd64-bf: Brainfuck for systems programmers

This is an extension of brainfuck to facilitate system programming.
It includes a link editor, `bold`, and a dynamic loader, `ld-bf.so`.
For now, it works exclusively on amd64 (aka x86_64) machines.

At program startup all cells will contain zero, except for the
cell directly left of the starting cell which will contain its
own address.

Accessing cells further left results in undefined behaviour.

All cells are 64 bits wide.

Command reference:

    +   Increase value of current cell
    -   Decrease value of current cell
    >   Go right
    <   Go left
    [   Start loop
    ]   End loop
    .   Write Character to stdout
    ,   Read Character from stdin
    !   Do a syscall and store the result in the current cell.
        Before doing the call, the values of the next seven cells 
        beginning at the current are stored into %rax, %rdi, %rsi,
        %rdx, %r10, %r8 and %r9, in that order.
    =   Copy cell contents. Can be chained. 
        (i.e. === -> copy value of current cell 3 cells to the right)
    *   Interpret cell value as memory addres, and load

# FAQ

### Q: Why not just use a compiler?

A: After compiling, we are left with assembler which is hard to read for 
   many people. With this method the original source code will be preserved,
   which makes debugging easier.

### Q: Raw syscalls are annoying, how about a way to call C functions?

A: By reusing library code, bugs and bad design choices made by the library implementer are
   propagated into user code. The only way to ensure bug-free and maintainable code is
   to write everything from scratch.

### Q: What are the benefits of having a dynamic loader that doesn't do relocations and works completely statically?

A: Please, take a look at this picture of a cute kitten:

![](./kitty.jpg)

(Picture by [Mel](https://www.flickr.com/photos/karamell/), CC BY-ND 2.0)



# Remarks

* Thread-local storage isn't properly implemented (yet?)

* `bold` should probably support a `-static` flag. Or any flag at all.

* An `install` target is still missing. An autotools-based configury seems like a good match ;)

* I will be very impressed if someone manages to write a program that
  properly daemonizes and accepts incoming connections on a socket.