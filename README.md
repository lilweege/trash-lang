# trash-lang

My first programming language written from scratch in C

NOTE: This is very much an unfinished work in progess (as evident by the mess of code in `src`). It only barely works with a few of the examples. Many things are subject to change and improve.

## Features
- [x] Compiled
  - [x] x86_64 Linux NASM
  - [ ] x86_64 Windows NASM
- [x] Statically typed
  - [x] Integer arithmetic
  - [ ] Floating-point arithmetic
  - [ ] Strings
- [x] Loops (`while`)
- [x] Conditionals (`if`/`else`)
- [x] Arrays
  - [ ] Variable-length arrays
- [ ] User-defined subroutines
- [ ] User-defined structures
- [x] Write to `stdout`
- [ ] Read from `stdin`
- [ ] Command line arguments

## Quick Start


#### Unix

I have not tested this on a mac, so please let me know if you try. It should work otherwise on Linux.

- Run GNU `make`.

```console
$ make
$ ./bin/trash -r examples/hello1.trash
```

#### Windows

Any of the following are viable options for building the project:

- Run the `make.bat` script by typing `make` or `.\make.bat` from an msvc-enabled terminal.
- Run GNU `make` from within a WSL terminal.
- Run MinGW `make`.

```console
> make
> .\bin\trash.exe -r examples\hello1.trash
```