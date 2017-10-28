CC := gcc
PARSER := out/parser
TESTCASE_ALL = $(wildcard test/*.* test/*/*.*)

parser: src/syntax.y src/lexical.l src/parser.c src/AST.c
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y -d -v --locations -o out/syntax.tab.c
	$(CC) out/syntax.tab.c src/parser.c src/AST.c -lfl -ly -o $(PARSER)

scanner: src/lexical.l
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	$(CC) scanner.c out/lex.yy.c -lfl -o out/scanner

testall: parser
	@bash test.sh $(TESTCASE_ALL)

clean:
	@$(RM) -r out

.PHONY: clean testall
