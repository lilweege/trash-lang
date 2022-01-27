# trash-lang

My first programming language written from scratch in C

NOTE: This is very much an unfinished work in progess (as evident by the mess of code in `src`). It only barely works with a few of the examples. Many things are subject to change and improve.

## Features
- [x] Compiled
  - [x] x86_64 Linux NASM
  - [ ] x86_64 Windows NASM
- [x] Statically typed
  - [x] Integer arithmetic
  - [x] Floating-point arithmetic
  - [x] Strings
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


#### Unix / WSL

##### Compilation

[NASM](https://nasm.us/) is required to assemble the generated `.asm` file. Nasm can be installed via your package manager of choice (for instance `apt install nasm` on Debian).

```console
$ make
$ ./bin/trash -c examples/hello.trash
$ ./hello
```

##### Simulation

```console
$ make
$ ./bin/trash -r examples/hello.trash
```

#### Windows

Requirements
- msvc

##### Compilation

Windows is currently not a supported compilation target.

##### Simulation

- Run the `make.bat` script by typing `make` or `.\make.bat` from an msvc-enabled terminal.

```console
> .\make.bat
> .\bin\trash.exe -r examples\hello.trash
```