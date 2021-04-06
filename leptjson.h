#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h>  /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
    union { // 使用枚举可以节约空间
        struct { char* str; size_t len; }s;  /* string */
        double n;                         /* number */
    }u;
    lept_type type;
}lept_value;

#define TRUE 1
#define FALSE 0

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,       // 若一个 JSON 只含有空白
    LEPT_PARSE_INVALID_VALUE,      // 无效值
    LEPT_PARSE_ROOT_NOT_SINGULAR,  // 若一个值之后，在空白之后还有其他字符
    LEPT_PARSE_NUMBER_TOO_BIG,     // 数值超过DOUBLE值范围
    LEPT_PARSE_MISS_QUOTATION_MARK,    // 缺少引号
    LEPT_PARSE_INVALID_STRING_ESCAPE,  // 无效的转义 
    LEPT_PARSE_INVALID_STRING_CHAR,    // 无效的字符
    LEPT_PARSE_INVALID_UNICODE_HEX,        // \u之后不是4位十六进制数字
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,  // 缺少低代理项或low surrogate不在正确范围内
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while (0)

// 解析字符串
int lept_parse(lept_value* v, const char* json);

// 清空分配的内存
void lept_free(lept_value* v);

// 获取类型
lept_type lept_get_type(const lept_value* v);
// 设置为NULL
void lept_set_null(lept_value* v);

// 获取bool值
int lept_get_boolean(const lept_value* v);
// 设置bool值
void lept_set_boolean(lept_value* v, int b);

// 获取数值
double lept_get_number(const lept_value* v);
// 设置数值
void lept_set_number(lept_value* v, double n);

// 设置字符串
void lept_set_string(lept_value* v, const char* s, size_t len);
// 获取字符串
const char* lept_get_string(const lept_value* v);
// 获取字符串长度
size_t lept_get_string_length(const lept_value* v);


#endif /* LEPTJSON_H__ */
