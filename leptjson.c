/*=============================================================================
#  Author:           shihao - https://github.com/shihao-seu
#  Email:            shihao10Civil@163.com
#  FileName:         leptjson.c
#  Description:      None
#  Version:          0.0.1
#  CreatingDate:     2021-Apr-Tue
#  History:          None
#  Copyright 2021-2031 Shi Hao
=============================================================================*/

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
#define ARRARY_ERROR(ret) \
    do {\
        for (size_t i = 0; i < size; i++)\
            lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));\
        c->top = 0;\
        return ret;\
    } while (0)

#define OBJECT_ERROR(ret) \
    do {\
        lept_member* rm;\
        for (size_t i = 0; i < size; i++) {\
            rm = (lept_member*)lept_context_pop(c, sizeof(lept_member));\
            free(rm->k);\
            lept_free(&rm->v);\
        }\
        c->top = 0;\
        return ret;\
    } while (0)

#define PUTS(c, s, len)  memcpy(lept_context_push_len(c, len), s, len)


static void lept_context_init(lept_context* c, const char* json) {
    c->json = json;
    c->size = LEPT_PARSE_STACK_INIT_SIZE;
    c->stack = (char*)malloc(sizeof(char) * c->size);
    c->top = 0;
}

// 与一般push不同，这里压入len个字节，类型待定，因此用到void *指向该待赋值区域
static void* lept_context_push_len(lept_context* c, size_t len) {
    void* ret;
    while (c->size < c->top + len) {
        c->size += c->size >> 1;  // <=> *1.5
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top; // 压入字节流
    c->top += len;

    return ret;
}

static void lept_context_push(lept_context* c, const char ch) {
    // 如果溢栈，需要重新分配内存
    if (c->size < c->top + 1) {
        c->size += c->size >> 1;  // <=> *1.5
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
#endif  // 0

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

    // strtod: 从字符串中解析数字, 第二个参数若不为NULL则指向下一个字符
    v->u.n = strtod(c->json, NULL);
    if (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL)
        return LEPT_PARSE_NUMBER_TOO_BIG;

    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

//  解析 4 位十六进制整数为码点
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

// code refactoring：extract method, c->json => c->stack => *str
// 注意这里用到了双指针，因为要改变指针的值，而单指针只能改变指向的元素
static int lept_parse_string_raw(lept_context* c, char** str, size_t* size) {
    EXPECT(c, '\"');
    const char* p = c->json;
    // 如果数组里包含字符串，那么c->stack只用到后半部分，前半部分是lept_value
    // 因此需要记录栈的进入点、栈的使用长度
    size_t head = c->top; 
    unsigned u, L;
    while (1) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                *size = c->top - head;
                // c->stack只能临时存放字符串，迟早要拷贝到新的字符串，否则free(c->stack)会销毁掉字符串
                memcpy(*str = (char*)malloc((*size + 1) * sizeof(char)), lept_context_pop(c, *size), *size);
                // WARN：如果改成*str[*size] = '\0'; 将是一个严重的BUG，
                //      根据运算符结合律，该式等价于*(str[*size])显然偏离原意
                (*str)[*size] = '\0'; // ！！！字符串拷贝不要忘了末尾的空字符。或者写作：*(*str + *size) = '\0';
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

// 解析字符串，c->json => c->stack => v->u.s.str
static int lept_parse_string(lept_context* c, lept_value* v) {
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK) {
        lept_set_string(v, s, len);
        free(s);
    }
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v); /* 前向声明 */

// 解析数组
// literal/num：c->json => e => c->stack前端, 
// 字符串/数组：    c->json => c->stack后端 => e => c->stack前端
// 最终，      c->satck前端 => v->u.a.e
static int lept_parse_array(lept_context* c, lept_value* v) {
    size_t size = 0;
    lept_value e;
    int ret;
    EXPECT(c, '[');
    lept_parse_whitespace(c);  // 跳空白
    if (*c->json == ']') {
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }
    while (1) {
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
            // 栈内存在临时值，需要弹出并释放，且栈顶归零
            ARRARY_ERROR(ret);
        // 压入结构体lept_value：lept_context_push_len是在c.stack中预留一个lept_value大小的空白空间，然后用memcpy给它赋值
        memcpy(lept_context_push_len(c, sizeof(lept_value)), &e, sizeof(lept_value));
        size++;
        /*!!! 误以为要释放e.s.str或者e.a.e，但是c->stack[n].s.str或者c->stack[n].a.e也指向同一块区域
         *    因此无需释放e，因为即便e被销毁，这些heap-alloced memory仍然有c内部指针指向它们。但要注意在最后释放掉*/
        // lept_free(&e); 
        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        } else if (*c->json == ']') {
            c->json++;
            lept_set_array(v, size * 2); // 初始capacity有一倍的冗余
            v->u.a.size = size;
            size *= sizeof(lept_value);
            // !!! 通过memcpy，c->stack[n].s.str或者c->stack[n].a.e指向的分配空间
            //     又转交给了v->u.a.e[n]管理，最终要在lept_free()中释放
            memcpy(v->u.a.e, lept_context_pop(c, size), size);
            return LEPT_PARSE_OK;
        } else
            ARRARY_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET);
    }
}

// 解析对象
// key: c->json => c->stack后端 => m.k
// value: c->json => m.v
// 最终，m => c.stack前端 => v.u.o.m
static int lept_parse_object(lept_context* c, lept_value* v) {
    size_t size = 0;
    lept_member m;
    int ret;
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = LEPT_OBJECT;
        v->u.o.m = NULL;
        v->u.o.size = 0;
        return LEPT_PARSE_OK;
    }
    while (1) {
        lept_init(&m.v);
        m.k = NULL;/* ownership is transferred to member on stack */
        /* parse key to m.k, m.klen */
        if (*c->json != '\"') {
            free(m.k);
            OBJECT_ERROR(LEPT_PARSE_MISS_KEY);
        }
        if ((ret = lept_parse_string_raw(c, &m.k, &m.klen)) != LEPT_PARSE_OK) {
            free(m.k);
            OBJECT_ERROR(LEPT_PARSE_MISS_KEY);
        }
        /* parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json == ':') {
            c->json++;
            lept_parse_whitespace(c);
        } else {
            free(m.k);
            OBJECT_ERROR(LEPT_PARSE_MISS_COLON);
        }
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            OBJECT_ERROR(ret);
        memcpy(lept_context_push_len(c, sizeof(lept_member)), &m, sizeof(lept_member));
        size++;

        lept_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            lept_parse_whitespace(c);
        } else if (*c->json == '}') {
            c->json++;
            lept_set_object(v, size * 2);
            v->u.o.size = size;
            size *= sizeof(lept_member);
            memcpy(v->u.o.m, lept_context_pop(c, size), size);
            return LEPT_PARSE_OK;
        } else
            OBJECT_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET);
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
        case '[':   return lept_parse_array(c,v);
        case '{':   return lept_parse_object(c, v);
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
            nRet = LEPT_PARSE_ROOT_NOT_SINGULAR;
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
    else if (v->type == LEPT_ARRAY) {
        // 释放每一个lept_value内部分配的内存
        for (size_t i = 0; i < v->u.a.size; i++)
            lept_free(&v->u.a.e[i]);
        free(v->u.a.e);
    } else if (v->type == LEPT_OBJECT) {
        for (size_t i = 0; i < v->u.o.size; i++) {
            free(v->u.o.m[i].k);
            lept_free(&v->u.o.m[i].v);
        }
        free(v->u.o.m);
    }
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
    v->u.s.size = len;
    v->type = LEPT_STRING;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.str;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.size;
}

size_t lept_get_array_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

// 原始数组在解析完的大小是固定的，现在修改其数据结构为动态数组，类似于vector
// 也就是初始分配一个大的空间，解析时在里面加入元素，超过capacity再realloc内存
void lept_set_array(lept_value* v, size_t capacity) {
    assert(v != NULL);
    lept_free(v);
    v->type = LEPT_ARRAY;
    v->u.a.size = 0;
    v->u.a.capacity = capacity;
    v->u.a.e = capacity > 0 ? (lept_value*)malloc(capacity * sizeof(lept_value)) : NULL;
    if (v->u.a.e)
        for (size_t i = 0; i < capacity; i++)
            lept_init(&v->u.a.e[i]);
}

size_t lept_get_array_capacity(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.capacity;
}

void lept_reserve_array(lept_value* v, size_t capacity) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.capacity < capacity) {
        v->u.a.capacity = capacity;
        v->u.a.e = (lept_value*)realloc(v->u.a.e, capacity * sizeof(lept_value));
    }
}

void lept_shrink_array(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.capacity > v->u.a.size) {
        v->u.a.capacity = v->u.a.size;
        v->u.a.e = (lept_value*)realloc(v->u.a.e, v->u.a.capacity * sizeof(lept_value));
    }
}

lept_value* lept_pushback_array_element(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (v->u.a.size == v->u.a.capacity)
        // 若容量为 0，则分配 1 个元素；其他情况倍增容量
        lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);
    lept_init(&v->u.a.e[v->u.a.size]);
    return &v->u.a.e[v->u.a.size++];
}

void lept_popback_array_element(lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY && v->u.a.size > 0);
    lept_free(&v->u.a.e[--v->u.a.size]);
}

lept_value* lept_insert_array_element(lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (index >= v->u.a.size)
        return lept_pushback_array_element(v);
    if (v->u.a.size == v->u.a.capacity)
        lept_reserve_array(v, v->u.a.capacity * 2);
    for (size_t i = v->u.a.size; i > index; i--)
        memcpy(v->u.a.e + i, v->u.a.e + i - 1, sizeof(lept_value));
    v->u.a.size++;
    return &v->u.a.e[index];
}

void lept_erase_array_element(lept_value* v, size_t index, size_t count) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    if (index >= v->u.a.size)
        return;
    size_t limit = count < v->u.a.size - index ? count : v->u.a.size - index;
    for (size_t i = 0; i < limit; i++)
        lept_free(&v->u.a.e[index + i]);
    if (limit)
        for (size_t j = index + limit; j < v->u.a.size; j++)
            memcpy(v->u.a.e + j - limit, v->u.a.e + j, sizeof(lept_value));
    v->u.a.size -= limit;
}

void lept_clear_array(lept_value* v) {
    lept_erase_array_element(v, 0, v->u.a.size);
}

size_t lept_get_object_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.size;
}

const char* lept_get_object_key(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].k;
}

size_t lept_get_object_key_length(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return v->u.o.m[index].klen;
}

lept_value* lept_get_object_value(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    assert(index < v->u.o.size);
    return &v->u.o.m[index].v;
}

size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen) {
    size_t i;
    assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
    for (i = 0; i < v->u.o.size; i++)
        if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
            return i;
    return LEPT_KEY_NOT_EXIST;
}

lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen) {
    size_t index = lept_find_object_index(v, key, klen);
    return index != LEPT_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
}

lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen) {
    assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
    size_t index = lept_find_object_index(v, key, klen);
    if (index != LEPT_KEY_NOT_EXIST)
        return &v->u.o.m[index].v;
    else {  // add a new key-value pair
        if (v->u.o.size == v->u.o.capacity)
            lept_reserve_object(v, v->u.o.capacity == 0 ? 1 : v->u.o.capacity * 2);
        lept_member* m = &v->u.o.m[v->u.o.size++];
        memcpy(m->k = (char*)malloc(klen + 1), key, klen);
        m->k[klen] = '\0';
        m->klen = klen;
        lept_init(&m->v);
        return &m->v;
    }
}

void lept_set_object(lept_value* v, size_t capacity) {
    assert(v != NULL);
    lept_free(v);
    v->type = LEPT_OBJECT;
    v->u.o.size = 0;
    v->u.o.capacity = capacity;
    v->u.o.m = capacity > 0 ? (lept_member*)malloc(capacity * sizeof(lept_member)) : NULL;
    if (v->u.o.m)
        for (size_t i = 0; i < capacity; i++)
            lept_init(&v->u.o.m[i].v);
}

size_t lept_get_object_capacity(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    return v->u.o.capacity;
}

void lept_reserve_object(lept_value* v, size_t capacity) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    if (v->u.o.capacity < capacity) {
        v->u.o.capacity = capacity;
        v->u.o.m = (lept_member*)realloc(v->u.a.e, capacity * sizeof(lept_member));
    }
}

void lept_shrink_object(lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    if (v->u.o.capacity > v->u.a.size) {
        v->u.o.capacity = v->u.a.size;
        v->u.o.m = (lept_member*)realloc(v->u.a.e, v->u.o.capacity * sizeof(lept_member));
    }
}

void lept_remove_object_value_index(lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_OBJECT && index < v->u.o.size);
    free(v->u.o.m[index].k);
    lept_free(&v->u.o.m[index].v);
    for (size_t i = index; i < v->u.o.size - 1; i++)
        memcpy(&v->u.o.m[i], &v->u.o.m[i + 1], sizeof(lept_member));
    v->u.o.size--;
}

void lept_remove_object_value_key(lept_value* v, const char* key, size_t klen) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    size_t index = lept_find_object_index(v, key, klen);
    if (index)
        lept_remove_object_value_index(v, index);
}

void lept_clear_object(lept_value* v) {
    assert(v != NULL && v->type == LEPT_OBJECT);
    for (size_t i = 0; i < v->u.o.size; i++) {
        free(v->u.o.m[i].k);
        lept_free(&v->u.o.m[i].v);
    }
    v->u.o.size = 0;
}

void lept_stringify_string_deprecated(lept_context* c, const char* str, size_t size) {
    assert(str != NULL);
    lept_context_push(c, '\"');  // 首尾压入 \"
    for (size_t i = 0; i < size; i++) {
        switch (str[i]) {
            case '\"': PUTS(c, "\\\"", 2); break;
            case '\\': PUTS(c, "\\\\", 2); break;
            case '\b': PUTS(c, "\\b", 2);  break;
            case '\f': PUTS(c, "\\f", 2);  break;
            case '\n': PUTS(c, "\\n", 2);  break;
            case '\r': PUTS(c, "\\r", 2);  break;
            case '\t': PUTS(c, "\\t", 2);  break;
            default: 
                if (str[i] < 0x20) {
                    char buffer[7];
                    sprintf(buffer, "\\u%04X", str[i]);
                    PUTS(c, buffer, 6);
                } else
                    lept_context_push(c, str[i]);
        }
    }
    lept_context_push(c, '\"');
}

// 优化1：提前分配足够的内存，避免了lept_context_push_len中的频繁检查内存是否充足的负担，以空间换时间
// 优化2：手动编写十六进制输出，避免了调用 sprinf 的内存开销
static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
    static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t i, size;
    char* head, *p;
    assert(s != NULL);
    p = head = lept_context_push_len(c, size = len * 6 + 2); /* "\u00xx..." */
    *p++ = '"';
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': *p++ = '\\'; *p++ = '\"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b';  break;
            case '\f': *p++ = '\\'; *p++ = 'f';  break;
            case '\n': *p++ = '\\'; *p++ = 'n';  break;
            case '\r': *p++ = '\\'; *p++ = 'r';  break;
            case '\t': *p++ = '\\'; *p++ = 't';  break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    c->top -= size - (p - head);
}

static int lept_stringify_value(lept_context* c, const lept_value* v) {
    size_t i;
    switch (v->type) {
        case LEPT_NULL:   PUTS(c, "null",  4); break;
        case LEPT_FALSE:  PUTS(c, "false", 5); break;
        case LEPT_TRUE:   PUTS(c, "true",  4); break;
        case LEPT_NUMBER:
            c->top -= 32 - sprintf(lept_context_push_len(c, 32), "%.17g", v->u.n);
            break;
        case LEPT_STRING: lept_stringify_string(c, v->u.s.str, v->u.s.size); break;
        case LEPT_ARRAY: 
            lept_context_push(c, '[');
            for (i = 0; i < v->u.a.size; i++) {
                if(i) lept_context_push(c, ',');
                lept_stringify_value(c, &v->u.a.e[i]);
            }
            lept_context_push(c, ']');
            break;
        case LEPT_OBJECT: {
            lept_context_push(c, '{');
            lept_member* m;
            for (i = 0; i < v->u.o.size; i++) {
                if (i) lept_context_push(c, ',');
                m = v->u.o.m + i;
                lept_stringify_string(c, m->k, m->klen);
                lept_context_push(c, ':');
                lept_stringify_value(c, &m->v);
            }
            lept_context_push(c, '}');
        }
            break;
    }
    return LEPT_STRINGIFY_OK;
}

int lept_stringify(const lept_value* v, char** json, size_t* length) {
    lept_context c;
    int ret;
    assert(v != NULL);
    assert(json != NULL);
    c.stack = (char*)malloc(c.size = LEPT_PARSE_STACK_INIT_SIZE);
    c.top = 0;
    if ((ret = lept_stringify_value(&c, v)) != LEPT_STRINGIFY_OK) {
        free(c.stack);
        *json = NULL;
        return ret;
    }
    if (length)
        *length = c.top;
    c.stack[c.top] = '\0';
    *json = c.stack;
    return LEPT_STRINGIFY_OK;
}

int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if (lhs->type != rhs->type)
        return FALSE;
    switch (lhs->type) {
        case LEPT_STRING:
            return lhs->u.s.size == rhs->u.s.size && 
                    memcmp(lhs->u.s.str, rhs->u.s.str, lhs->u.s.size) == 0;
        case LEPT_NUMBER:
            return lhs->u.n == rhs->u.n;
        case LEPT_ARRAY:
            if (lhs->u.a.size != rhs->u.a.size)
                return FALSE;
            for (size_t i = 0; i < lhs->u.a.size; i++)
                if (!lept_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
                    return FALSE;
            return TRUE;
        case LEPT_OBJECT: {
            if (lhs->u.o.size != rhs->u.o.size)
                return FALSE;
            const char* key;
            size_t klen;
            lept_value *lv, *rv;
            for (size_t i = 0; i < lhs->u.o.size; i++) {
                key = lept_get_object_key(lhs, i);
                klen = lept_get_object_key_length(lhs, i);
                if (!(rv = lept_find_object_value(rhs, key, klen)))
                    return FALSE;
                lv = &(lhs->u.o.m[i].v);
                if (!lept_is_equal(lv, rv))
                    return FALSE;
            }
            return TRUE;
        }
        default: return TRUE;
    }
}

void lept_copy(lept_value* dst, const lept_value* src) {
    assert(src != NULL && dst != NULL && src != dst);
    switch (src->type) {
        case LEPT_STRING:
            lept_set_string(dst, src->u.s.str, src->u.s.size);
            break;
        case LEPT_ARRAY:
            lept_set_array(dst, src->u.a.capacity);
            dst->u.a.size = src->u.a.size;
            for (size_t i = 0; i < src->u.a.size; i++)
                lept_copy(&dst->u.a.e[i], &src->u.a.e[i]);
            break;
        case LEPT_OBJECT:
            lept_set_object(dst, src->u.o.capacity);
            dst->u.o.size = src->u.o.size;
            for (size_t i = 0; i < src->u.o.size; i++) {
                dst->u.o.m[i].k = (char*)malloc(src->u.o.m[i].klen + 1);
                memcpy(dst->u.o.m[i].k, src->u.o.m[i].k, src->u.o.m[i].klen);
                dst->u.o.m[i].k[src->u.o.m[i].klen] = '\0';
                dst->u.o.m[i].klen = src->u.o.m[i].klen;
                lept_copy(&dst->u.o.m[i].v, &src->u.o.m[i].v);
            }
            break;
        default:
            lept_free(dst);
            memcpy(dst, src, sizeof(lept_value));
            break;
    }
}

void lept_move(lept_value* dst, lept_value* src) {
    assert(dst != NULL && src != NULL && src != dst);
    lept_free(dst);
    memcpy(dst, src, sizeof(lept_value));
    lept_init(src);
}

void lept_swap(lept_value* lhs, lept_value* rhs) {
    assert(lhs != NULL && rhs != NULL);
    if (lhs != rhs) {
        lept_value temp;
        memcpy(&temp, lhs, sizeof(lept_value));
        memcpy(lhs,   rhs, sizeof(lept_value));
        memcpy(rhs, &temp, sizeof(lept_value));
    }
}