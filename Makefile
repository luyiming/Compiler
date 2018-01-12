CC := gcc
CFLAGS := -lfl -ly -I./include -std=gnu11 -g
CSOURCE := src/AST.c src/semantic.c src/common.c src/rb_tree.c src/sym_table.c src/ir.c src/oc.c
BFLAGS := -d -v --locations

ifneq ($(OS),Windows_NT)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        CFLAGS += -I/usr/local/opt/flex/include -L/usr/local/opt/flex/lib -L/usr/local/opt/bison/lib
    endif
endif

parser: ir

semantic_check:
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y $(BFLAGS) -o out/syntax.tab.c
	$(CC) -D ENABLE_NESTED_SCOPE out/syntax.tab.c src/semantic_check.c $(CSOURCE) $(CFLAGS) -o out/semantic_check

gen_ir:
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y $(BFLAGS) -o out/syntax.tab.c
	$(CC) out/syntax.tab.c src/gen_ir.c $(CSOURCE) $(CFLAGS) -o out/gen_ir

gen_raw_ir:
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y $(BFLAGS) -o out/syntax.tab.c
	$(CC) -D NO_OPTIMIZE out/syntax.tab.c src/gen_ir.c $(CSOURCE) $(CFLAGS) -o out/gen_ir

gen_oc:
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y $(BFLAGS) -o out/syntax.tab.c
	$(CC) out/syntax.tab.c src/gen_oc.c $(CSOURCE) $(CFLAGS) -o out/gen_oc

testall: test_semantic_check test_gen_ir

test_semantic_check: semantic_check
	@python3 test.py out/semantic_check test/semantic_test

test_gen_ir: gen_ir


clean:
	@$(RM) -r out

.PHONY: clean testall
