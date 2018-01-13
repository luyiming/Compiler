[![Build Status](https://travis-ci.com/luyiming/Compiler.svg?token=ZqSWSJNAyq6N8GY6iVps&branch=master)](https://travis-ci.com/luyiming/Compiler)

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

## Usage
### 1. Generate Abstract Syntax Tree (AST)

build
```
make gen_AST
```

usage:

```
out/gen_AST path-to-source-file [output-path]
```

sample AST output

```
Program (1)
  ExtDefList (1)
    ExtDef (1)
      Specifier (1)
        TYPE: int
      FunDec (1)
        ID: main
        LP
        RP
      CompSt (2)
        LC
        DefList (3)
          Def (3)
            Specifier (3)
              TYPE: float
            DecList (3)
              Dec (3)
                VarDec (3)
                  ID: i
                ASSIGNOP
                Exp (3)
                  FLOAT: 0.000105
            SEMI
        RC
```

### 2. Semantic analysis

build
```bash
make semantic_check
```

usage:

```
out/semantic_check path-to-source-file
```

### 3. Generate IR

build

```bash
make gen_ir [output-path]       # with optimization
make gen_raw_ir [output-path]   # without optimization
```

usage:

```
out/gen_ir path-to-source-file
```

Optimization techniques:
- Copy propagation
- Constant propagation
- Constant folding
- Dead code elimination
- Redundant label elimination
- Other heuristic algorithms

sample IR output

```
FUNCTION main :
READ t1
v_n := t1
t2 := v_n
t3 := #0
IF t2 <= t3 GOTO label1
t4 := #1
WRITE t4
GOTO label2
LABEL label1 :
t5 := v_n
t6 := #0
IF t5 >= t6 GOTO label3
t8 := #1
t7 := #0 - t8
WRITE t7
GOTO label4
LABEL label3 :
t9 := #0
WRITE t9
LABEL label4 :
LABEL label2 :
t10 := #0
RETURN t10
```

### 4. generate MIPS code

build

```bash
make gen_oc
```

usage:

```
out/gen_oc path-to-source-file [output-path]
```

sample MIPS code output

```
.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text
read:
li $v0, 4
la $a0, _prompt
syscall
li $v0, 5
syscall
jr $ra

write:
li $v0, 1
syscall
li $v0, 4
la $a0, _ret
syscall
move $v0, $0
jr $ra

main:
li $t5, 0
li $t4, 1
li $t3, 0
addi $sp, $sp, -4
sw $ra, 0($sp)
jal read
lw $ra, 0($sp)
addi $sp, $sp, 4
move $t1, $v0
move $t2, $t1
label1:
blt $t3, $t2, label2
j label3
label2:
add $t1, $t5, $t4
move $a0, $t4
addi $sp, $sp, -4
sw $ra, 0($sp)
jal write
lw $ra, 0($sp)
addi $sp, $sp, 4
move $t5, $t4
move $t4, $t1
addi $t1, $t3, 1
move $t3, $t1
j label1
label3:
move $v0, $0
jr $ra
```