#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <stdarg.h> 

#include "parser.h"
#include "test_helpers.h"

typedef union {
    int int_value;
    char bool_value;
    char *str_value;
} expression_value;

void test_expression(struct expression *e, expression_value expected);

void assert_parser_errors(struct parser *p) {
    if (p->errors > 0) {
        printf("parser has %d errors: \n", p->errors);
        for (int i = 0; i < p->errors; i++) {
            printf("  - %s\n", p->error_messages[i]);
        }

        exit(1);
    }
}

void assert_program_size(struct program *p, unsigned expected_size) {
    if (p->size != expected_size) {
        abortf("wrong program size. expected %d, got %d\n", expected_size, p->size);
    }   
}

void test_let_statements() {
    char *input = ""
        "let x = 5;\n"
        "let y = true;\n"
        "let foo = y;\n";

    struct lexer l = {input, 0 };
    struct parser parser = new_parser(&l);
    struct program *program = parse_program(&parser);
    
    assert_parser_errors(&parser);
    assert_program_size(program, 3);    

    struct test {
        char *literal;
        char *name;
        expression_value value;
    } tests[3] = {
        {"let", "x", {.int_value = 5}},
        {"let", "y", {.bool_value = 1}},
        {"let", "foo", {.str_value = "y"}}
    };

    for (int i = 0; i < 3; i++) {
        struct statement stmt = program->statements[i];
        assertf(strcmp(stmt.token.literal, tests[i].literal) == 0, "wrong literal. expected %s, got %s\n", tests[i].literal, stmt.token.literal);
        assertf(strcmp(stmt.name.value, tests[i].name) == 0, "wrong name value. expected %s, got %s\n", tests[i].name, stmt.name.value);
        assertf(strcmp(stmt.name.token.literal, tests[i].name) == 0, "wrong name literal. expected %s, got %s", tests[i].name, stmt.token.literal);
        test_expression(stmt.value, tests[i].value);
    }

    free_program(program);
}


void test_return_statements() {
    char *input = ""
        "return 5;\n"
        "return true;\n"
        "return x;\n";

    struct lexer l = {input, 0 };
    struct parser parser = new_parser(&l);
    struct program *program = parse_program(&parser);
    
    assert_parser_errors(&parser);
    assert_program_size(program, 3);   

    struct test {
        char *literal;
        char *name;
        expression_value value;
    } tests[3] = {
        {"return", "", {.int_value = 5}},
        {"return", "", {.bool_value = 1}},
        {"return", "", {.str_value = "x"}}
    };

    for (int i = 0; i < 3; i++) {
        struct statement stmt = program->statements[i];
        assertf(strcmp(stmt.token.literal, tests[i].literal) == 0, "wrong literal. expected %s, got %s\n", tests[i].literal, stmt.token.literal);
        test_expression(stmt.value, tests[i].value);
    }

    free_program(program);
}

void test_program_string() {
    struct expression e1 = {
        .type = EXPR_INT,
        .integer = {
            .value = 5,
            .token = {
                .literal = "5",
                .type = TOKEN_INT,
            }
        }
    };
    struct expression e2 = {
        .type = EXPR_IDENT,
        .ident = {
            .value = "foo"
        },
    };
    struct expression expressions[] = {
        {
            .type = EXPR_IDENT,
            .ident = {
                .token = {
                    .type = TOKEN_IDENT,
                    .literal = "anotherVar",
                },
                .value = "anotherVar"
            }
        },
        {
            .type = EXPR_INFIX,
            .infix = {
                .operator = "+",
                .left = &e1,
                .right = &e2,
            }
        }
    };
    struct statement statements[] = {
        {
            .type = STMT_LET,
            .token = {
                .type = TOKEN_LET,
                .literal = "let",
            },
            .name = {
                .token = {
                    .type = TOKEN_IDENT,
                    .literal = "myVar",
                },
                .value = "myVar",
            },
            .value = &expressions[0],
        }, 
        {
            .type = STMT_RETURN,
            .token = {
                .type = TOKEN_RETURN,
                .literal = "return",
            },
            .value = &expressions[1]
        }, 
    };

    struct program program = {
        .statements = statements, 
        .size = 2
    };

    char *str = program_to_str(&program);
    char *expected = "let myVar = anotherVar;return (5 + foo);";
    assertf(strcmp(str, expected) == 0, "wrong program string. expected \"%s\", got \"%s\"\n", expected, str);
    free(str);
}

void test_identifier_expression(struct expression *e, char *expected) {
    assertf(e->type == EXPR_IDENT, "wrong expression type: expected %d, got %d\n", EXPR_IDENT, e->type);
    assertf(strcmp(e->ident.token.literal, expected) == 0, "wrong token literal: expected \"%s\", got \"%s\"\n", expected, e->ident.token.literal);
    assertf(strcmp(e->ident.value, expected) == 0, "wrong expression value: expected \"%s\", got \"%s\"\n", expected, e->ident.value);
}


void test_identifier_expression_parsing() {
    char *input = "foobar;";
    struct lexer l = {input, 0};
    struct parser parser = new_parser(&l);
    struct program *program = parse_program(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    assertf(stmt.token.type == TOKEN_IDENT, "wrong token type: expected %s, got %s\n", token_to_str(TOKEN_IDENT), stmt.token.type);
    assertf(strcmp(stmt.token.literal, "foobar") == 0, "wrong token literal: expected %s, got %s\n", "foobar", stmt.token.literal);
    test_identifier_expression(stmt.value, "foobar");
    free_program(program);
}


void test_integer_expression(struct expression *expr, int expected) {
    assertf(expr->type == EXPR_INT, "wrong expression type: expected %d, got %d\n", EXPR_INT, expr->type);
    assertf(expr->integer.value == expected, "wrong integer value: expected %d, got %d\n", expected, expr->integer.value);

    char expected_str[8];
    sprintf(expected_str, "%d", expected);
    assertf(strcmp(expr->integer.token.literal, expected_str) == 0, "wrong token literal: expected %s, got %s\n", expected_str, expr->integer.token.literal);
}


void test_integer_expression_parsing() {
    char *input = "5;";
    struct lexer l = {input, 0};
    struct parser parser = new_parser(&l);
    struct program *program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    assertf(stmt.token.type == TOKEN_INT, "wroken token type: expected %s, got %s", token_to_str(TOKEN_INT), stmt.token.type);
    assertf (strcmp(stmt.token.literal, "5") == 0, "wrong token literal: expected %s, got %s", "foobar", stmt.token.literal);
    test_integer_expression(stmt.value, 5);
    free_program(program);
}


void test_boolean_expression(struct expression * expr, char expected) {
    assertf(expr->type == EXPR_BOOL, "wrong expression type: expected %d, got %d\n", EXPR_BOOL, expr->type);
    assertf(expr->bool.value == expected, "wrong boolean value: expected %d, got %d\n", expected, expr->bool.value);
    
    char *expected_str = expected ? "true" : "false";
    assertf(strcmp(expr->bool.token.literal, expected_str) == 0, "wrong token literal: expected %s, got %s\n", expected_str, expr->bool.token.literal);
}

void test_boolean_expression_parsing() {
    struct test {
        char * input;
        char expected;
    } tests[] = {
      {"true;", 1},
      {"false;", 0}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program *program = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(program, 1);
        struct statement stmt = program->statements[0];

        test_boolean_expression(stmt.value, tests[i].expected);
        free_program(program);
    }
}

void test_expression(struct expression *e, expression_value expected) {
    switch (e->type) {
        case EXPR_BOOL: return test_boolean_expression(e, expected.bool_value); break;
        case EXPR_INT: return test_integer_expression(e, expected.int_value); break;
        case EXPR_IDENT: return test_identifier_expression(e, expected.str_value); break;
        default: break;
    }
}

void test_infix_expression(struct expression *expr, expression_value left_value, char *operator, expression_value right_value) {
    assertf(expr->type == EXPR_INFIX, "wrong expression type. expected %d, got %d\n", EXPR_INFIX, expr->type);
    test_expression(expr->infix.left, left_value);
    assertf(strncmp(expr->infix.operator, operator, MAX_OPERATOR_LENGTH) == 0, "wrong operator: expected %s, got %s\n", operator, expr->infix.operator);
    test_expression(expr->infix.right, right_value);
}

void test_infix_expression_parsing() {
    struct test{
        char *input;
        expression_value left_value;
        char *operator;
        expression_value right_value;        
    };
    struct test tests[] = {
       {"5 + 5", {5}, "+", {5}},
       {"5 - 5", {5}, "-", {5}},
       {"5 * 5", {5}, "*", {5}},
       {"5 / 5", {5}, "/", {5}},
       {"5 > 5", {5}, ">", {5}},
       {"5 < 5", {5}, "<", {5}},
       {"5 == 5", {5}, "==", {5}},
       {"5 != 5", {5}, "!=", {5}},
       {"true == true", {1}, "==", {1}},
       {"true != false", {1}, "!=", {0}},
       {"false == false", {0}, "==", {0}},
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct test t = tests[i];
        struct lexer l = new_lexer(t.input);
        struct parser parser = new_parser(&l);
        struct program *program = parse_program(&parser);
        assert_parser_errors(&parser);
        assert_program_size(program, 1);
        struct statement stmt = program->statements[0];
        test_infix_expression(stmt.value, t.left_value, t.operator, t.right_value);
        free_program(program);   
    }
}

void test_prefix_expression_parsing() {
    typedef struct test {
        char *input;
        char *operator;
        expression_value value;
    } test;

    test tests[] = {
        {"!5", "!", { .int_value = 5 }},
        {"-15", "-", { .int_value = 15 }},
        {"!true", "!", { .bool_value = 1 }},
        {"!false", "!", { .bool_value = 0 }},
    };
    test t;
    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        t = tests[i];
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program *program = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(program, 1);
        struct statement stmt = program->statements[0];

        assertf(stmt.value->type == EXPR_PREFIX, "wrong expression type. expected %d, got %d\n", EXPR_PREFIX, stmt.value->type);
        assertf(strncmp(stmt.value->prefix.operator, t.operator, 2) == 0, "wrong operator. expected %s, got %s\n", t.operator, stmt.value->prefix.operator);
        test_expression(stmt.value->prefix.right, t.value); 
        free_program(program);       
    }
}

void test_operator_precedence_parsing() {
    struct test {
        char *input;
        char *expected;
    } tests[] = {
       {"-a * b", "((-a) * b)"},
       {"!-a", "(!(-a))"},
       {"a + b + c", "((a + b) + c)"},
       {"a + b - c", "((a + b) - c)"},
       {"a * b * c", "((a * b) * c)"},
       {"a * b / c", "((a * b) / c)"},
       {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
       {"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
       {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
       {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
       {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
       {"true", "true"},
       {"false", "false"},
       {"3 > 5 == false", "((3 > 5) == false)"},
       {"3 < 5 == true", "((3 < 5) == true)"},
       {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
       {"(5 + 5) * 2", "((5 + 5) * 2)"},
       {"2 / ( 5 + 5)", "(2 / (5 + 5))"},
       {"-(5 + 5)", "(-(5 + 5))"},
       {"!(true == true)", "(!(true == true))"},
       {"a + add(b * c) +d", "((a + add((b * c))) + d)"},
       {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7* 8))", "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
       {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program *program = parse_program(&parser);
        assert_parser_errors(&parser);
        
        char *program_str = program_to_str(program);
        assertf(strncmp(program_str, tests[i].expected, 48) == 0, "wrong program string: expected %s, got %s\n", tests[i].expected, program_str);        
        free(program_str);
        free_program(program);
    }
}

void test_if_expression_parsing() {
    char *input = "if (x < y) { x }";
    struct lexer lexer = {input};
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    struct expression *expr = stmt.value;
    assertf (expr->type == EXPR_IF, "invalid statement type: expected %d, got %d\n", EXPR_IF, stmt.type);

    expression_value left = {.str_value = "x"};
    expression_value right = {.str_value = "y"};
    test_infix_expression(expr->ifelse.condition, left, "<", right);

    struct block_statement *consequence = expr->ifelse.consequence;
    assertf(!!consequence, "expected consequence block statement, got NULL\n");
    assertf(consequence->size == 1, "invalid consequence size: expected %d, got %d\n", 1, consequence->size);
    assertf(consequence->statements[0].type == STMT_EXPR, "statements[0] is not a statement expression, got %d\n", consequence->statements[0].type);
    test_identifier_expression(consequence->statements[0].value, "x");
    assertf(!expr->ifelse.alternative, "expected NULL, got alternative block statement\n");
    free_program(program);
}

void test_if_else_expression_parsing() {
    char *input = "if (x < y) { x } else { 5 }";
    struct lexer lexer = {input, 0};
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    struct expression *expr = stmt.value;
    assertf(expr->type == EXPR_IF, "invalid statement type: expected %d, got %d\n", EXPR_IF, stmt.type);

    expression_value left = {.str_value = "x"};
    expression_value right = {.str_value = "y"};
    test_infix_expression(expr->ifelse.condition, left, "<", right);

    struct block_statement *consequence = expr->ifelse.consequence;
    assertf(!!consequence, "expected consequence block statement, got NULL\n");
    assertf(consequence->size == 1, "invalid consequence size: expected %d, got %d\n", 1, consequence->size);
    assertf(consequence->statements[0].type == STMT_EXPR, "statements[0] is not a statement expression, got %d", consequence->statements[0].type);
    test_identifier_expression(consequence->statements[0].value, "x");

    struct block_statement *alternative = expr->ifelse.alternative;
    assertf(alternative != NULL, "expected alternative, got NULL");
    assertf(alternative->size == 1, "invalid alternative size: expected %d, got %d\n", 1, alternative->size);
    free_program(program);
}

void test_function_literal_parsing() {
    char *input = "fn(x, y) { x + y; }";
    struct lexer lexer = {input, 0};
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    assertf(stmt.type == STMT_EXPR, "invalid statement type. expected STMT_EXPR, got %d\n", stmt.type);

    struct expression *expr = stmt.value;
    assertf(expr->type == EXPR_FUNCTION, "invalid expression type: expected EXPR_FUNCTION, got %d\n", expr->type);
    
    assertf(expr->function.parameters.size == 2, "invalid param size: expected %d, got %d\n", 2, expr->function.parameters.size);
    assertf(strcmp(expr->function.parameters.values[0].value, "x") == 0, "invalid parameter[0]: expected %s, got %s\n", "x", expr->function.parameters.values[0].value);
    assertf(strcmp(expr->function.parameters.values[1].value, "y") == 0, "invalid parameter[0]: expected %s, got %s\n", "x", expr->function.parameters.values[1].value);
    assertf(expr->function.body->size == 1, "invalid body size: expected %d, got %d\n", 1, expr->function.body->size);
    
    expression_value left = {.str_value = "x"};
    char *op = "+";
    expression_value right = {.str_value = "y"};
    test_infix_expression(expr->function.body->statements[0].value, left, op, right);
    free_program(program);
}

void test_call_expression_parsing() {
    char *input = "add(1, 2 * 3, 4 + 5);";
    struct lexer lexer = {input, 0};
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(program, 1);

    struct statement stmt = program->statements[0];
    assertf(stmt.type == STMT_EXPR, "invalid statement type. expected STMT_EXPR, got %d\n", stmt.type);

    struct expression *expr = stmt.value;
    assertf(expr->type == EXPR_CALL, "invalid expression type: expected EXPR_CALL, got %d\n", expr->type);
    test_identifier_expression(expr->call.function, "add");
    assertf(expr->call.arguments.size == 3, "expected 3 arguments, got %d\n", expr->call.arguments.size);

    expression_value left = {.int_value = 2};
    expression_value right = {.int_value = 3};
    test_infix_expression(&expr->call.arguments.values[1], left, "*", right);
    test_infix_expression(&expr->call.arguments.values[2], (expression_value) 4, "+", (expression_value) 5);
    free_program(program);
}

// TODO: Add test for argument parsing
// TODO: Add test for parameter parsing

int main() {
    test_let_statements();
    test_return_statements();
    test_program_string();
    test_identifier_expression_parsing();
    test_integer_expression_parsing();
    test_boolean_expression_parsing();
    test_prefix_expression_parsing();
    test_infix_expression_parsing();
    test_operator_precedence_parsing();
    test_if_expression_parsing();
    test_if_else_expression_parsing();
    test_function_literal_parsing();
    test_call_expression_parsing();
    printf("\x1b[32mAll parsing tests passed!\033[0m\n");
}