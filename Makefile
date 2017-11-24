CC := gcc
CFLAGS := -lfl -ly -I./include
BFLAGS := -d -v --locations
PARSER := out/parser
TESTCASE_ALL = $(wildcard test/pretest1/*.*)

parser: src/syntax.y src/lexical.l src/parser.c src/AST.c
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y $(BFLAGS) -o out/syntax.tab.c
	$(CC) out/syntax.tab.c src/parser.c src/AST.c $(CFLAGS) -o $(PARSER)

testall: parser
	@bash test.sh $(TESTCASE_ALL)

clean:
	@$(RM) -r out

.PHONY: clean testall
