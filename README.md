# Compiler for C--

# Building Source

## Prerequisites

The following libraries are required to build:

- [flex](https://github.com/westes/flex)
- [bison](https://www.gnu.org/software/bison/)

Please make sure that all dependancies are installed and linked properly to your PATH before building.

### Linux
If you use Debian/Ubuntu, simply cut'n paste:
```bash
sudo apt-get install flex bison
```

## Build Instructions

### Linux
```bash
make parser
```
