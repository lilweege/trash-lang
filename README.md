# V2 IS IN PROGRESS, THIS BRANCH WILL BE DELETED


# trash-lang

My first programming language written from scratch in C

## Features
- [x] Compiled
  - [x] x86_64 Linux (NASM / GNU ld)
  - [ ] x86_64 Windows (NASM / MSVC link)
- [x] Statically typed
  - [x] Integer arithmetic
  - [x] Floating-point arithmetic
  - [x] Strings
- [x] Loops (`while`)
- [x] Conditionals (`if`/`else`)
- [x] Arrays
  - [ ] Constexpr-length arrays
  - [ ] Variable-length arrays
- [ ] User-defined procedures
- [ ] User-defined structures
- [x] Write to `stdout`
- [ ] Read from `stdin`
- [ ] Command line arguments

## Quick Start

#### Linux / WSL2

##### Compilation

[NASM](https://nasm.us/) and [GNU ld](https://www.gnu.org/software/binutils/) are required to assemble and link the generated `.asm` file. `nasm` can be installed via your package manager of choice (for instance `apt install nasm` on Debian), `ld` is included with the GNU binutils.

```console
$ make
$ ./bin/trash -c examples/hello.trash
$ ./hello.out
```

##### Simulation (deprecated)

```console
$ make
$ ./bin/trash -r examples/hello.trash
```

#### Windows

##### Compilation

Windows is currently not a supported compilation target.

##### Simulation (deprecated)

Run the `make.bat` script by typing `make` or `.\make.bat` from an msvc-enabled terminal.

```console
> .\make.bat
> .\bin\trash.exe -r examples\hello.trash
```
