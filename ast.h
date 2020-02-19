#include "lexer.h"
#include <stdlib.h>

#define MAX_IDENT_LENGTH 32
#define MAX_OPERATOR_LENGTH 3
enum {
    LOWEST = 1,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL            // fn()
};

typedef enum {
    EXPR_INFIX,
    EXPR_PREFIX,
    EXPR_INT,
    EXPR_IDENT,
    EXPR_BOOL,
   // EXPR_IF,
} expression_type;

typedef struct {
    token token;
    char value;
} bool_expression;

typedef struct {
    token token;
    char value[MAX_IDENT_LENGTH];
} identifier_expression;

typedef struct {
    token token;
    int value;
} integer_expression;

typedef struct {
    token token;
    char operator[MAX_OPERATOR_LENGTH];
    struct expression *right;
} prefix_expression;

typedef struct {
    token token;
    char operator[MAX_OPERATOR_LENGTH];
    struct expression *left;
    struct expression *right;
} infix_expression;

typedef struct expression {
    expression_type type;
    union {
        bool_expression bool;
        identifier_expression ident;
        integer_expression _int;
        prefix_expression prefix;
        infix_expression infix;
    };
} expression;

typedef struct {
    token token; 
    char value[MAX_IDENT_LENGTH];
} identifier;

typedef struct {
    token token;
    identifier name;
    expression * value;
} statement;

typedef struct {
    token token;
    statement *statements;
} block_statement;

typedef struct {
    token token;
    expression condition;
    block_statement *consequence;
    block_statement *alternative;
} if_expression;

typedef struct program {
    statement * statements;
    unsigned int cap;
    unsigned int size;
} program;

typedef struct parser {
    lexer * lexer;
    token current_token;
    token next_token;

    // TODO: allocate this dynamically
    unsigned int errors;
    char error_messages[8][128];
} parser;

static expression * parse_expression(parser *p, int precedence);
static int get_token_precedence(token t);
static void next_token(parser * p);
static int current_token_is(parser *p, int t);
static int next_token_is(parser *p, int t) ;
static int expect_next_token(parser *p, int t);

parser new_parser(lexer *l) {
    parser p = {
        .lexer = l,
    };
   
    // read two tokens so that both current_token and next_token are set
    next_token(&p);
    next_token(&p);
    return p;
}

static void next_token(parser * p) {
    p->current_token = p->next_token;
    gettoken(p->lexer, &p->next_token);
}

static int current_token_is(parser *p, int t) {
    return t == p->current_token.type;
}

static int next_token_is(parser *p, int t) {
     return t == p->next_token.type;
}

static int expect_next_token(parser *p, int t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return 1;
    }

    sprintf(p->error_messages[p->errors++], "expected next token to be %s, got %s instead", token_to_str(t), token_to_str(p->next_token.type));
    return 0;
}

static int parse_let_statement(parser *p, statement *s) {
    s->token = p->current_token;

     if (!expect_next_token(p, IDENT)) {
        return -1;
    }

    // parse name
    identifier id = {
        .token = p->current_token,
    };
    strcpy(id.value, p->current_token.literal);
    s->name = id;

    if (!expect_next_token(p, ASSIGN)) {
        return -1;
    }

    // TODO: Read expression here, for now we just skip forward until semicolon

    while (!current_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static int parse_return_statement(parser *p, statement *s) {
    s->token = p->current_token;

    next_token(p);

    // TODO: Parse expression

    while (!current_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

expression *parse_identifier_expression(parser *p) {
    expression *expr = malloc(sizeof (expression));
    expr->type = EXPR_IDENT;
    expr->ident.token = p->current_token;
    strncpy(expr->ident.value, p->current_token.literal, MAX_IDENT_LENGTH);
    return expr;
}

expression *parse_int_expression(parser *p) {
    expression *expr = malloc(sizeof (expression));
    expr->type = EXPR_INT;
    expr->_int.token = p->current_token;
    expr->_int.value =  atoi(p->current_token.literal);
    return expr;
}

expression *parse_prefix_expression(parser *p) {
    expression *expr = malloc(sizeof (expression));
    expr->type = EXPR_PREFIX;
    expr->prefix.token = p->current_token;
    strncpy(expr->prefix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    next_token(p);
    expr->prefix.right = parse_expression(p, PREFIX);
    return expr;
}

static expression *parse_infix_expression(parser *p, expression *left) {
    expression * expr = malloc(sizeof (expression));
    expr->type = EXPR_INFIX;
    expr->infix.left = left;
    expr->infix.token = p->current_token;
    strncpy(expr->infix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->infix.right =  parse_expression(p, precedence);
    return expr;
}

expression *parse_boolean_expression(parser *p) {
    expression *expr = malloc(sizeof (expression));
    expr->type = EXPR_BOOL;
    expr->bool.token = p->current_token;
    expr->bool.value = current_token_is(p, TRUE);
    return expr;
}

expression *parse_grouped_expression(parser *p) {
    next_token(p);
    
    expression *expr = parse_expression(p, LOWEST);

    if (!expect_next_token(p, RPAREN)) {
        free(expr);
        return NULL;
    }

    return expr;
}

static expression *parse_expression(parser *p, int precedence) {
    expression *left;
    switch (p->current_token.type) {
        case IDENT: 
            left = parse_identifier_expression(p); 
            break;
        case INT: 
            left = parse_int_expression(p); 
            break;
        case BANG:
        case MINUS: 
            left = parse_prefix_expression(p);
             break;
        case TRUE:
        case FALSE: 
            left = parse_boolean_expression(p); 
            break;
        case LPAREN:
            left = parse_grouped_expression(p);
            break;
        default: 
            sprintf(p->error_messages[p->errors++], "no prefix parse function found for %s", token_to_str(p->current_token.type));
            return NULL;
        break;
    }

    while (!next_token_is(p, SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        int type = p->next_token.type;
        if (type == PLUS || type == MINUS || type == ASTERISK || type == SLASH || type == EQ || type == NOT_EQ || type == LT || type == GT) {
            next_token(p);
            left = parse_infix_expression(p, left);
        } else {
            return left;
        }
    }

    return left;
}

static int get_token_precedence(token t) {
    switch (t.type) {
        case EQ: return EQUALS;
        case NOT_EQ: return EQUALS;
        case LT: return LESSGREATER;
        case GT: return LESSGREATER;
        case PLUS: return SUM;
        case MINUS: return SUM;
        case SLASH: return PRODUCT;
        case ASTERISK: return PRODUCT;
    }

    return LOWEST;
};

static int parse_expression_statement(parser *p, statement *s) {
    s->token = p->current_token;
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, SEMICOLON)) {
        next_token(p);
    } 

    return 1;
}

static int parse_statement(parser *p, statement *s) {
    switch (p->current_token.type) {
        case LET: return parse_let_statement(p, s); break;
        case RETURN: return parse_return_statement(p ,s); break;
        default: return parse_expression_statement(p, s); break;
    }
  
   return -1;
}

extern program parse_program(parser *parser) {
    program prog = {
        .size = 0,
        .cap = 1,
    };
    prog.statements = malloc(sizeof (statement));

    statement s;
    while (parser->current_token.type != EOF) {
        // if an error occured, skip token & continue
        if (parse_statement(parser, &s) == -1) {
            next_token(parser);
            continue;
        }

        // add to program statements
        prog.statements[prog.size++] = s;

        // increase program capacity if needed
        if (prog.size >= prog.cap) {
            prog.statements = realloc(prog.statements, sizeof (statement) * prog.cap * 2);
            prog.cap *= 2;
        }

        next_token(parser);        
    }

    return prog;
}


static char * let_statement_to_str(statement s) {
    char * str = malloc(sizeof(s.token.literal) + sizeof(s.name.token.literal) + sizeof(s.value->ident.token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.name.token.literal);
    strcat(str, " = ");
    strcat(str, s.value->ident.token.literal);
    strcat(str, ";");
    return str;
}

static char * return_statement_to_str(statement s) {
    char * str = malloc(sizeof(str) + sizeof(s.token.literal) + sizeof(s.value->ident.token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.value->ident.token.literal);
    strcat(str, ";");
    return str;
}

static char * expression_to_str(expression *expr) {
    char * str = malloc(256);

    switch (expr->type) {
        case EXPR_PREFIX: 
            sprintf(str, "(%s%s)", expr->prefix.operator, expression_to_str(expr->prefix.right)); 
        break;
        case EXPR_INFIX: 
            sprintf(str, "(%s %s %s)", expression_to_str(expr->infix.left), expr->infix.operator, expression_to_str(expr->infix.right));
        break;
        case EXPR_IDENT:
            strcpy(str, expr->ident.value);
        break;
        case EXPR_BOOL:
            strcpy(str, expr->bool.value ? "true" : "false");
            break;
        case EXPR_INT:
            strcpy(str, expr->_int.token.literal);
        break;
    }

    return str;
}

char * program_to_str(program *p) {
    char * str = malloc(256);
    *str = '\0';

    statement s;

    for (int i = 0; i < p->size; i++) {
        s = p->statements[i];
               
        switch (s.token.type) {
            case LET: 
                strcat(str, let_statement_to_str(s)); 
            break;
            case RETURN: 
                strcat(str, return_statement_to_str(s)); 
            break;
            default:
                strcat(str, expression_to_str(s.value));
            break;
        }
        
        if (i < p->size -1) {
            str = realloc(str, sizeof str + 256);
        }
    }    

    return str;
}