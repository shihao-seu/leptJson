#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>   /* errno */
#include <math.h>    /* HUGE_VAL */
#include <string.h>  /* memcpy */
#include <stdio.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256  // 栈初始大小
#endif
/*
    当程序以 release 配置编译时（定义了 NDEBUG 宏），assert() 不会做检测；
而当在 debug 配置时，则会在运行时检测 assert(cond) 中的条件是否为真（非 0），
断言失败会直接令程序崩溃。
*/
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while (0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISHEXDIGIT(ch)      (ISDIGIT(ch) || ((ch) >= 'a' && (ch) <= 'f') || ((ch) >= 'A' && (ch) <= 'F'))
#define ISWHITESPACE(ch)    ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r')
#define STRING_ERROR(ret)   do { c->top = 0; return ret; } while (0) 


typedef struct {
    const char* json;
    char* stack;
    size_t size, top; // 栈最大值、顶层位置
}lept_context;

static void lept_context_init(lept_context* c, const char* json) {
    c->json = json;
    c->size = LEPT_PARSE_STACK_INIT_SIZE;
    c->stack = (char*)malloc(sizeof(char) * c->size);
    c->top = 0;
}

static void lept_context_push(lept_context* c, const char ch) {
    // 如果溢栈，需要重新分配内存
    if (c->size < c->top + 1) {
        c->size += c->size >> 1; // <=> *1.5
        c->stack = (char*)realloc(c->stack, c->size);
    }
    c->stack[(c->top)++] = ch;
}

// 与一般pop不同，这里弹出前len个值
static char* lept_context_pop(lept_context* c, size_t len) {
    assert(c->top >= len);
    c->top -= len;
    return c->stack + c->top;
}

// 跳过空白符
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (ISWHITESPACE(*p))
        p++;
    c->json = p;
}
#if 0
// 解析"null"
int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

// 解析"true"
int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

// 解析"false"
int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}
#endif // 0

// 代码重构，整合lept_parse_null/true/false
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    EXPECT(c, literal[0]);
    size_t i = 0;
    for (; literal[i+1]; i++)
        if (c->json[i] != literal[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

// 解析数字(参考ECMA404-num构成图)
static int lepr_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* strod识别的数字范围超过json格式要求，需要进行限制 */
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else if (ISDIGIT1TO9(*p))
        for (p++; ISDIGIT(*p); p++) ;
    else
        return LEPT_PARSE_INVALID_VALUE;
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p))
            return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++) ;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++) ;
    }
    // !!注意,最后需要限制末尾字符串必须为空
    if (!ISWHITESPACE(*p) && *p != '\0')
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    
    // strod: 从字符串中解析数字，end指向有效数之后的字符
    v->u.n = strtod(c->json, &end);  
    if (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)
        return LEPT_PARSE_NUMBER_TOO_BIG;

    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

//解析 4 位十六进制整数为码点
static const char* lept_parse_hex4(const char* p, unsigned* u) {
    *u = 0;
    unsigned v = 0;
    for (size_t i = 4; i; i--) {
        if (!ISHEXDIGIT(*p))
            return NULL;
        v = (*p >= 'a') ? (*p - 'a' + 10) : ((*p >= 'A') ? (*p - 'A' + 10) : (*p - '0'));
        *u += v << (4*(i-1));
        p++;
    }
    return p;
}

// 把这个码点编码成 UTF-8
static void lept_encode_utf8(lept_context* c, unsigned u) {
    assert(u >= 0x0000 && u <= 0x10FFFF);
    if (u <= 0x7F) {
        lept_context_push(c, u & 0xFF);
    } else if (u <= 0x7FF) {
        lept_context_push(c, 0xC0 | ((u >> 6) & 0xFF));
        lept_context_push(c, 0x80 | ( u       & 0x3F));
    } else if (u <= 0xFFFF) {
        lept_context_push(c, 0xE0 | ((u >> 12) & 0xFF)); /* 0xE0 = 11100000 */
        lept_context_push(c, 0x80 | ((u >>  6) & 0x3F)); /* 0x80 = 10000000 */
        lept_context_push(c, 0x80 | ( u        & 0x3F)); /* 0x3F = 00111111 */
    } else {
        lept_context_push(c, 0xF0 | ((u >> 18) & 0xFF));
        lept_context_push(c, 0x80 | ((u >> 12) & 0x3F));
        lept_context_push(c, 0x80 | ((u >>  6) & 0x3F));
        lept_context_push(c, 0x80 | ( u        & 0x3F));
    }
}

// 解析字符串
static int lept_parse_string(lept_context* c, lept_value* v) {
    EXPECT(c, '\"');
    const char* p = c->json;
    unsigned u, L;
    while (1) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                lept_set_string(v, lept_context_pop(c, c->top), c->top);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
            case '\\':
                switch (*p++) {
                    case '\"':lept_context_push(c, '\"'); break;
                    case '\\':lept_context_push(c, '\\'); break;
                    case '/': lept_context_push(c,  '/'); break;
                    case 'b': lept_context_push(c, '\b'); break;
                    case 'f': lept_context_push(c, '\f'); break;
                    case 'n': lept_context_push(c, '\n'); break;
                    case 'r': lept_context_push(c, '\r'); break;
                    case 't': lept_context_push(c, '\t'); break;
                    case 'u':
                        if (!(p = lept_parse_hex4(p, &u)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        // 处理代理对surrogate pair
                        if ( u >= 0xD800 && u <= 0xDBFF ) {
                            if ( *p++ != '\\' )
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if ( *p++ != 'u' )
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);                     
                            p = lept_parse_hex4(p, &L);
                            if (!p || L < 0xDC00 || L > 0xDFFF )
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            u = 0x10000 + (u - 0xD800) * 0x400 + (L - 0xDC00);
                        }
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }
                break; // !!! 小小break，容易忽视，问题多多
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                lept_context_push(c, ch);
        }
    }  
}

// 解析第一个非空字符
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case '\0':  return LEPT_PARSE_EXPECT_VALUE;  // 纯空白行
        case 't':   return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':   return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':   return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '"':   return lept_parse_string(c, v);
        default:    return lepr_parse_number(c, v);
    }
}

int lept_parse(lept_value* v, const char* json) {
    assert(v != NULL);
    lept_init(v);
    lept_context c;
    lept_context_init(&c, json);
    lept_parse_whitespace(&c);           // 处理第一部分
    int nRet = lept_parse_value(&c, v);  // 处理第二部分
    if ( LEPT_PARSE_OK == nRet ) {       // 处理第三部分
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return nRet;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.str);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

void lept_set_null(lept_value* v) {
    lept_free(v);
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    assert(v != NULL && b >=0 );
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    assert(v != NULL);
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.str = (char*)malloc(len + 1);
    memcpy(v->u.s.str, s, len);
    v->u.s.str[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.str;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}