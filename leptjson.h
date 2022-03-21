/*=============================================================================
#  Author:           shihao - https://github.com/shihao-seu
#  Email:            shihao10Civil@163.com
#  FileName:         leptjson.h
#  Description:      leptjson 的头文件，含有对外的类型和API函数声明
#  Version:          0.0.1
#  CreatingDate:     2021-Apr-Tue
#  History:          None
=============================================================================*/

#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h>  /* size_t */

/**
 * @brief: json包含7种数据类型：object, array, number, string, true, false, or null,用枚举表示
 */
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct t_lept_member lept_member;
typedef struct t_lept_value lept_value;

/**
 * @brief：json值，JSON 文本被解析为一个树状数据结构
 */
struct t_lept_value{
    union {
        struct { lept_member* m; size_t size, capacity; }o; /* obeject*/
        struct { char* str; size_t size; }s;                /* string */
        struct { lept_value* e; size_t size, capacity; }a;  /* array  */
        double n;                                           /* number */
    } u;
    lept_type type;
};


/**
 * @brief：
 */
struct t_lept_member {
    char* k; size_t klen;   /* member key string, key string length */
    lept_value v;           /* member value */
};


/**
 * @brief：
 */
typedef struct {
    const char* json;
    char* stack;
    size_t size, top; // 栈最大值、顶层位置
} lept_context;


/**
 * @brief：接口调用返回结果
 */
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,                 // 若一个 JSON 只含有空白
    LEPT_PARSE_INVALID_VALUE,                // 无效值
    LEPT_PARSE_ROOT_NOT_SINGULAR,            // 若一个值之后，在空白之后还有其他字符
    LEPT_PARSE_NUMBER_TOO_BIG,               // 数值超过DOUBLE值范围
    LEPT_PARSE_MISS_QUOTATION_MARK,          // 缺少引号
    LEPT_PARSE_INVALID_STRING_ESCAPE,        // 无效的转义 
    LEPT_PARSE_INVALID_STRING_CHAR,          // 无效的字符
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


/**
 * @brief 解析C风格字符串，转换成JSON值
 * 
 * @param [out] v: 程序可读结构体
 * @param [in] json: 字符串指针
 * @return int : 解析结果
 */
int lept_parse(lept_value* v, const char* json);


/**
 * @brief 清空内部分配内存
 * 
 * @param [in] v 
 */
void lept_free(lept_value* v);


/**
 * @brief 比较两个json是否相等
 * 
 * @param [in] lhs 
 * @param [in] rhs 
 * @return int : TRUE/FALSE
 */
int lept_is_equal(const lept_value* lhs, const lept_value* rhs);


/**
 * @brief 深拷贝lept_value
 * 
 * @param [out] dst 
 * @param [in] src 
 */
void lept_copy(lept_value* dst, const lept_value* src);


/**
 * @brief 移动语义
 * 
 * @param dst 
 * @param src 
 */
void lept_move(lept_value* dst, lept_value* src);


/**
 * @brief 交换操作
 * 
 * @param lhs 
 * @param rhs 
 */
void lept_swap(lept_value* lhs, lept_value* rhs);


/**
 * @brief 获取json值类型
 * 
 * @param v 
 * @return lept_type 
 */
lept_type lept_get_type(const lept_value* v);


/**
 * @brief 设置为NULL类型
 * 
 * @param v 
 */
void lept_set_null(lept_value* v);


/**
 * @brief 获取bool值
 * 
 * @param v 
 * @return int 
 */
int lept_get_boolean(const lept_value* v);


/**
 * @brief 设置bool值
 * 
 * @param v 
 * @param b 
 */
void lept_set_boolean(lept_value* v, int b);


/**
 * @brief 获取数值
 * 
 * @param [in] v: json值 
 * @return double 
 */
double lept_get_number(const lept_value* v);


/**
 * @brief 设置数值
 * 
 * @param [out] v: json值
 * @param n 
 */
void lept_set_number(lept_value* v, double n);


/**
 * @brief 设置字符串
 * 
 * @param [out] v: json值
 * @param [in] s: 输出字符串 
 * @param [in] len: 字符串长度 
 */
void lept_set_string(lept_value* v, const char* s, size_t len);


/**
 * @brief 获取字符串
 * 
 * @param [in] v: json值 
 * @return const char* 
 */
const char* lept_get_string(const lept_value* v);


/**
 * @brief 获取字符串长度
 * 
 * @param [in] v: json值  
 * @return size_t 
 */
size_t lept_get_string_length(const lept_value* v);


/**
 * @brief 获取数组的大小
 * 
 * @param [in] v: json值  
 * @return size_t 
 */
size_t lept_get_array_size(const lept_value* v);


/**
 * @brief 获取数组的元素
 * 
 * @param [in] v: json值  
 * @param index 
 * @return lept_value* 
 */
lept_value* lept_get_array_element(const lept_value* v, size_t index);


/**
 * @brief 设置数组初始容量
 * 
 * @param [in] v: json值  
 * @param capacity 
 */
void lept_set_array(lept_value* v, size_t capacity);


/**
 * @brief 获取数组容量大小
 * 
 * @param [in] v: json值  
 * @return size_t 
 */
size_t lept_get_array_capacity(const lept_value* v);

/**
 * @brief 调整数组容量大小，类似于vector.reserve()
 * 
 * @param [in] v: json值  
 * @param capacity 
 */
void lept_reserve_array(lept_value* v, size_t capacity);


/**
 * @brief 将capacity减少为size相同大小，类似于vector.shrink_to_fit()
 * 
 * @param [in] v: json值  
 */
void lept_shrink_array(lept_value* v);


/**
 * @brief 数组尾后添加新元素
 * 
 * @param v 
 * @return lept_value* 
 */
lept_value* lept_pushback_array_element(lept_value* v);


/**
 * @brief 数组弹出尾后元素
 * 
 * @param v 
 */
void lept_popback_array_element(lept_value* v);


/**
 * @brief 在 index 位置插入一个元素
 * 
 * @param v 
 * @param index 
 * @return lept_value* 
 */
lept_value* lept_insert_array_element(lept_value* v, size_t index);


/**
 * @brief 删去在 index 位置开始共 count 个元素（不改容量）
 * 
 * @param v 
 * @param index 
 * @param count 
 */
void lept_erase_array_element(lept_value* v, size_t index, size_t count);


/**
 * @brief 清除所有元素（不改容量）
 * 
 * @param v 
 */
void lept_clear_array(lept_value* v);


/**
 * @brief 获取对象大小
 * 
 * @param v 
 * @return size_t 
 */
size_t lept_get_object_size(const lept_value* v);


/**
 * @brief 获取对象成员键
 * 
 * @param v 
 * @param index 
 * @return const char* 
 */
const char* lept_get_object_key(const lept_value* v, size_t index);


/**
 * @brief 获取对象成员键长
 * 
 * @param v 
 * @param index 
 * @return size_t 
 */
size_t lept_get_object_key_length(const lept_value* v, size_t index);


/**
 * @brief 获取对象成员值
 * 
 * @param v 
 * @param index 
 * @return lept_value* 
 */
lept_value* lept_get_object_value(const lept_value* v, size_t index);


/**
 * @brief 按key寻找obj的key
 * 
 * @param v 
 * @param key 
 * @param klen 
 * @return size_t 
 */
size_t lept_find_object_index(const lept_value* v, const char* key, size_t klen);


/**
 * @brief 按key寻找obj的value
 * 
 * @param v 
 * @param key 
 * @param klen 
 * @return lept_value* 
 */
lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen);


/**
 * @brief 设置obj初始容量
 * 
 * @param v 
 * @param capacity 
 */
void lept_set_object(lept_value* v, size_t capacity);


/**
 * @brief 获取obj容量大小
 * 
 * @param v 
 * @return size_t 
 */
size_t lept_get_object_capacity(const lept_value* v);


/**
 * @brief 调整obj容量大小，类似于vector.reserve()
 * 
 * @param v 
 * @param capacity 
 */
void lept_reserve_object(lept_value* v, size_t capacity);


/**
 * @brief 将capacity减少为size相同大小，类似于vector.shrink_to_fit()
 * 
 * @param v 
 */
void lept_shrink_object(lept_value* v);


/**
 * @brief 根据key修改value,如果key不存在则添加一个key-value对
 * 
 * @param v 
 * @param key 
 * @param klen 
 * @return lept_value* 
 */
lept_value* lept_set_object_value(lept_value* v, const char* key, size_t klen);


/**
 * @brief 根据index删除键值对
 * 
 * @param v 
 * @param index 
 */
void lept_remove_object_value_index(lept_value* v, size_t index);


/**
 * @brief 根据键删除键值对
 * 
 * @param v 
 * @param key 
 * @param klen 
 */
void lept_remove_object_value_key(lept_value* v, const char* key, size_t klen);


/**
 * @brief 清除所有元素（不改容量）
 * 
 * @param v 
 */
void lept_clear_object(lept_value* v);


/**
 * @brief 生成JSON字符串
 * 
 * @param [in] v: json值
 * @param [out] json: 输出C字符串 
 * @param [out] length: 字符串长度 
 * @return int: 调用结果
 */
int lept_stringify(const lept_value* v, char** json, size_t* length);

#endif /* LEPTJSON_H__ */