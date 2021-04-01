#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,       
    LEPT_PARSE_EXPECT_VALUE,     // 若一个 JSON 只含有空白
    LEPT_PARSE_INVALID_VALUE,    // 无效值
    LEPT_PARSE_ROOT_NOT_SINGULAR // 若一个值之后，在空白之后还有其他字符
};

// 解析字符串
int lept_parse(lept_value* v, const char* json);

// 获取类型
lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */