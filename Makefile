CFLAGS+= -Werror -Wall -Isrc/ -g 
VPATH= src
TESTS= bin/lexer_test bin/parser_test bin/opcode_test bin/compiler_test bin/vm_test bin/symbol_table_test 

# disable crossjumping when using gcc so it doesn't optimize away our (optimized) dispatch table
ifeq "$(CC)" "gcc"
    CFLAGS+= -fno-gcse
endif

all: bin/pepper 

bin/:
	mkdir -p bin/

bin/pepper: pepper.c lexer.c parser.c opcode.c compiler.c object.c symbol_table.c builtins.c vm.c gc.c | bin/
	$(CC) $(CFLAGS) $^ -Ofast -o $@ 

# tests
bin/lexer_test: tests/lexer_test.c lexer.c | bin/
bin/parser_test: tests/parser_test.c parser.c lexer.c | bin/
bin/opcode_test: tests/opcode_test.c opcode.c | bin/
bin/compiler_test: tests/compiler_test.c lexer.c parser.c opcode.c compiler.c object.c symbol_table.c builtins.c | bin/
bin/vm_test: tests/vm_test.c lexer.c parser.c opcode.c compiler.c object.c symbol_table.c builtins.c vm.c gc.c | bin/
bin/symbol_table_test: tests/symbol_table_test.c symbol_table.c | bin/
bin/%_test: 
	$(CC) $(CFLAGS) -DTEST_MODE $^ -o $@

.PHONY: check
check: $(TESTS)
	for test in $^; do $$test || exit 1; done

.PHONY: memcheck
memcheck: $(TESTS)
	for test in $^; do valgrind $$test || exit 1; done

.PHONY: clean
clean:
	rm -r bin

.PHONY: install
install: bin/pepper
	cp bin/pepper /usr/local/bin/pepper
