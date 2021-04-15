#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h>  /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct t_lept_value lept_value;
typedef struct t_lept_member lept_member;

struct t_lept_value{
    union { // 使用枚举可以节约空间
        struct { lept_member* m; size_t size, capacity; }o; /* obeject*/
        struct { char* str; size_t size; }s;      /* string */
        struct { lept_value* e; size_t size, capacity; }a;  /* array  */
        double n;                                 /* number */
    }u;
    lept_type type;
};

struct t_lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
};

typedef struct {
    const char* json;
    char* stack;
    size_t size, top; // 栈最大值、顶层位置
}lept_context;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,       // 若一个 JSON 只含有空白
    LEPT_PARSE_INVALID_VALUE,      // 无效值
    LEPT_PARSE_ROOT_NOT_SINGULAR,  // 若一个值之后，在空白之后还有其他字符
    LEPT_PARSE_NUMBER_TOO_BIG,     // 数值超过DOUBLE值范围
    LEPT_PARSE_MISS_QUOTATION_MARK,    // 缺少引号
    LEPT_PARSE_INVALID_STRING_ESCAPE,  // 无效的转义 
    LEPT_PARSE_INVALID_STRING_CHAR,    // 无效的字符
    LEPT_PARSE_INVALID_UNICODE_HEX,          // \u之后不是4位十六进制数字
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,    // 缺少低代理项或low surrogate不在正确范围内
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, // 缺少逗号或者右方括号
    LEPT_PARSE_MISS_KEY,                     // 缺少键
    LEPT_PARSE_MISS_COLON,                   // 缺少冒号
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET,  // 缺少逗号或者右花括号
    LEPT_STRINGIFY_OK
};

#define LEPT_KEY_NOT_EXIST ((size_t)-1)
#define lept_init(v) do { (v)->type = LEPT_NULL; } while (0)
#define TRUE 1
#define FALSE 0

// 解析字符串
int lept_parse(lept_value* v, const char* json);

// 清空分配的内存
void lept_free(lept_value* v);
// 比较两个json是否相等
int lept_is_equal(const lept_value* lhs, const lept_value* rhs);
// 深度复制lept_value
void lept_copy(lept_value* dst, const lept_value* src);
// 移动操作
void lept_move(lept_value* dst, lept_value* src);
// 交换操作
void lept_swap(lept_value* lhs, lept_value* rhs);

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

// 获取数组的大小
size_t lept_get_array_size(const lept_value* v);
// 获取数组的元素
lept_value* lept_get_array_element(const lept_value* v, size_t index);
// 设置数组初始容量
void lept_set_array(lept_value* v, size_t capacity);
// 获取数组容量大小
size_t lept_get_array_capacity(const lept_value* v);
// 调整数组容量大小，类似于vector.reserve()
void lept_reserve_array(lept_value* v, size_t capacity);
// 将capacity减少为size相同大小，类似于vector.shrink_to_fit()
void lept_shrink_array(lept_value* v);
// 数组尾后添加新元素
lept_value* lept_pushback_array_element(lept_value* v);
// 数组弹出尾后元素
void lept_popback_array_element(lept_value* v);
// 在 index 位置插入一个元素
lept_value* lept_insert_array_element(lept_value* v, size_t index);
// 删去在 index 位置开始共 count 个元素（不改容量）
void lept_erase_array_element(lept_value* v, size_t index, size_t count);
// 清除所有元素（不改容量）
void lept_clear_array(lept_value* v);

// 获取对象大小
size_t lept_get_object_size(const lept_value* v);
// 获取对象成员键
const char* lept_get_object_key(const lept_value* v, size_t index);
// 获取对象成员键长
size_t lept_get_object_key_length(const lept_value* v, size_t index);
// 获取对象成员值
lept_value* lept_get_object_value(const lept_value* v, size_t index);
// 按key寻找obj的key
size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen);
// 按key寻找obj的value
lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen);
// 设置obj初始容量
void lept_set_object(lept_value* v, size_t capacity);
// 获取obj容量大小
size_t lept_get_object_capacity(const lept_value* v);
// 调整obj容量大小，类似于vector.reserve()
void lept_reserve_object(lept_value* v, size_t capacity);
// 将capacity减少为size相同大小，类似于vector.shrink_to_fit()
void lept_shrink_object(lept_value* v);
// 根据key修改value,如果key不存在则添加一个key-value对
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen);
// 根据index删除键值对
void lept_remove_object_value_index(lept_value* v, size_t index);
// 根据键删除键值对
void lept_remove_object_value_key(lept_value* v, const char* key, size_t klen);
// 清除所有元素（不改容量）
void lept_clear_object(lept_value* v);


// 生成json格式字符串
int lept_stringify(const lept_value* v, char** json, size_t* length);

#endif /* LEPTJSON_H__ */
