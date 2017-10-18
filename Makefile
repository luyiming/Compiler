CC := gcc

parser: src/syntax.y src/lexical.l src/parser.c
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	bison src/syntax.y -d -v --locations -o out/syntax.tab.c
	$(CC) out/syntax.tab.c src/parser.c -lfl -ly -o out/parser

scanner: src/lexical.l
	@mkdir -p out
	flex -o out/lex.yy.c src/lexical.l
	$(CC) scanner.c out/lex.yy.c -lfl -o out/scanner

clean:
	@$(RM) -r out

.PHONY: clean
