Statically-linked brainfuck
===========================

The language is standard brainfuck with added symbols to dereference
pointers and to do syscalls.

All cells are wide enough to hold a void*.

At program startup, the leftmost cell will contain its own address.
All other cells contain zero.

Reading and writing to cells to the left of the starting cell is
undefined behaviour.

    +   Increase value of current cell
    -   Decrease value of current cell
    >   Go right
    <   Go left
    [   Start loop
    ]   End loop
    .   Write Character to stdout
    ,   Read Character from stdin
    !   Do a syscall and store the result in the current cell.
        Before doing the call, the values of the next five cells
        are stored into %rax, %rdi, %rsi, %rdx, %r10, %r8, %r9,
        in that order.
    =   Copy cell contents. Can be chained. (=== -> copy 3 fields right)
    *   Load contents from address