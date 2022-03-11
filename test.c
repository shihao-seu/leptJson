/*=============================================================================
#  Author:           shihao - https://github.com/shihao-seu
#  Email:            shihao10Civil@163.com
#  FileName:         test.c
#  Description:      测试驱动开发。此文件包含测试程序，需要链接 leptjson 库。
#  Version:          0.0.1
#  CreatingDate:     2022-Mar-Fri
#  History:          None
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;    // 程序返回值
static int test_count = 0;  // 统计总的测试条目 
static int test_pass = 0;   // 成功测试条目
/*
* 定义一套测试框架：
* 先在头文件里确定API，
* 然后根据TDD思想实现这些API函数
*/

// 基本的测试框架，如果预取expect与实际actual相符（满足equality），则通过测试test_pass++，否则打印错误
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
        EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zd")


#define TEST_BASE(expect, json, types) \
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(expect, lept_parse(&v, json));\
        EXPECT_EQ_INT(types, lept_get_type(&v));\
        lept_free(&v);\
    } while (0)

#define TEST_NULL(json) TEST_BASE(LEPT_PARSE_OK, json, LEPT_NULL)
#define TEST_TRUE(json) TEST_BASE(LEPT_PARSE_OK, json, LEPT_TRUE)
#define TEST_FALSE(json) TEST_BASE(LEPT_PARSE_OK, json, LEPT_FALSE)
#define TEST_ERROR(error, json) TEST_BASE(error, json, LEPT_NULL)

#define TEST_NUMBER(nums, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(nums, lept_get_number(&v));\
        lept_free(&v);\
    } while (0)

#define TEST_STRING(str, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(str, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while (0)

// 解析字符串json得到lept_value，再生成一个新的字符串json2，二者比较，称之为往返测试
#define TEST_ROUNDTRIP(json)\
    do {\
        lept_value v;\
        char* json2;\
        size_t length;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRINGIFY_OK, lept_stringify(&v, &json2, &length));\
        EXPECT_EQ_STRING(json, json2, length);\
        lept_free(&v);\
        free(json2);\
    } while(0)

#define TEST_EQUAL(json1, json2, equality) \
    do {\
        lept_value v1, v2;\
        lept_init(&v1);\
        lept_init(&v2);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v1, json1));\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v2, json2));\
        EXPECT_EQ_INT(equality, lept_is_equal(&v1, &v2));\
        lept_free(&v1);\
        lept_free(&v2);\
    } while(0)


/**
 * @brief: 字面值测试，包括null、true、false
 * @notes: C语言中的static 函数：编译器会警告未被使用的static函数[-Wunused-function]
 */
static void test_parse_literal() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "   ");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");
    TEST_NULL("null");
    TEST_TRUE("true");
    TEST_FALSE("false");
}


/**
 * @brief: 数字类型测试
 * 
 */
static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-100000000000"); /* must underflow */
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}


/**
 * @brief : 测试无效数字类型
 * 
 */
static void test_parse_invalid_num() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "00.1");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "-00.1");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}


/**
 * @brief ：字符串测试
 * 
 */
static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("\" /", "\"\\\" \\/\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\x11World", "\"Hello\\u0011World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}


/**
 * @brief ：无效字符串类型
 * 
 */
static void test_parse_invalid_str() {
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\""); /* \v */
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\""); /* \' */
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\""); /* \0 */
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\""); /* \x12 */
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\""); /* %x01 */
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\""); /* %x1F */
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"\\u00A2");
}


/**
 * @brief ：无效unicode编码
 * 
 */
static void test_parse_invalid_unicode() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}


/**
 * @brief ：数组类型测试
 * 
 */
static void test_parse_array() {
    lept_value v;

    // 是否成功解析数组
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[  ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
    lept_free(&v);

    // 是否成功解析数组内元素
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
    lept_free(&v);

    // 嵌套数组测试
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    for (size_t i = 0; i < 4; i++) {
        lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
        for (size_t j = 0; j < i; j++) {
            lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));
        }
    }
    lept_free(&v);
}


/**
 * @brief 测试解析无效数组
 * 
 */
static void test_parse_invalid_array() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[\"a\", nul]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}


/**
 * @brief 测试解析对象类型
 * 
 */
static void test_parse_object() {
    lept_value v;
    size_t i;

    // 是否成功解析对象
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
    lept_free(&v);

    // 是否成功解析对象内的键值对
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));

    EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_object_value(&v, 0)));

    EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_object_value(&v, 1)));

    EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_object_value(&v, 2)));

    EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));

    EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)), lept_get_string_length(lept_get_object_value(&v, 4)));

    // 内部数组是否解析成功
    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
    }

    // 内部嵌套对象是否解析成功
    EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6));
    {
        lept_value* o = lept_get_object_value(&v, 6);
        EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
        for (i = 0; i < 3; i++) {
            lept_value* ov = lept_get_object_value(o, i);
            EXPECT_TRUE('1' + i == lept_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
        }
    }
    lept_free(&v);
}


/**
 * @brief 测试无效对象解析
 * 
 */
static void test_parse_invalid_obj() {
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}


/**
 * @brief 测试修改为NULL类型是否成功
 * 
 */
static void test_access_null() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_null(&v);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}


/**
 * @brief 测试修改为布尔类型是否成功
 * 
 */
static void test_access_boolean() {
    lept_value v;
    lept_init(&v);
    lept_set_null(&v);
    lept_set_boolean(&v, TRUE);
    EXPECT_TRUE(lept_get_boolean(&v));
    lept_set_boolean(&v, FALSE);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}


/**
 * @brief 测试修改为数字类型是否成功
 * 
 */
static void test_access_number() {
    lept_value v;
    lept_init(&v);
    lept_set_null(&v);
    lept_set_number(&v, -2.5);
    EXPECT_EQ_DOUBLE(-2.5, lept_get_number(&v));
    lept_free(&v);
}


/**
 * @brief 测试设置为字符串是否成功
 * 
 */
static void test_access_string() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "\" /", 3);
    EXPECT_EQ_STRING("\" /", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}


/**
 * @brief 测试数组类型的access接口
 * 
 */
static void test_access_arrary() {
// 测试一
    lept_value a1, a2, a, e;
    size_t size, i, j;
    lept_init(&a1);
    lept_init(&a2);
    lept_init(&a);
    lept_init(&e);

    // lept_parse(&v1, "[ 123, null , false , true , \"abc\", [4,\"shi\",6], { \"1\" : 1, \"2\" : 2, \"3\" : 3 } ]");
    lept_parse(&a1, "[ 123, null , false , true , \"abc\", [4,\"shi\",6]]");
    lept_set_array(&a2, lept_get_array_capacity(&a1));
    lept_set_number(&e, 123);

    /* test: lept_shrink_array\lept_get_array_capacity */
    lept_shrink_array(&a1);
    EXPECT_EQ_SIZE_T(lept_get_array_capacity(&a1), (size = lept_get_array_size(&a1)));

    /* test: lept_pushback_array_element\lept_insert_array_element */
    for (i = 1; i < size; i++)
        lept_copy(lept_pushback_array_element(&a2), lept_get_array_element(&a1, i));
    lept_move(lept_insert_array_element(&a2, 0), &e);
    EXPECT_EQ_INT(TRUE, lept_is_equal(&a1, &a2));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&e));

    /*test: lept_erase_array_element\lept_popback_array_element */
    lept_erase_array_element(&a1, 0, size);
    for (i = 0; i < size; i++)
        lept_popback_array_element(&a2);
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&a2));
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&a1));
    EXPECT_EQ_INT(TRUE, lept_is_equal(&a1, &a2));


// 测试二
    for (j = 0; j <= 5; j += 5) {
        lept_set_array(&a, j);
        EXPECT_EQ_SIZE_T(0, lept_get_array_size(&a));
        EXPECT_EQ_SIZE_T(j, lept_get_array_capacity(&a));
        for (i = 0; i < 10; i++) {
            lept_init(&e);
            lept_set_number(&e, i);
            lept_move(lept_pushback_array_element(&a), &e);
            lept_free(&e);
        }

        EXPECT_EQ_SIZE_T(10, lept_get_array_size(&a));
        for (i = 0; i < 10; i++)
            EXPECT_EQ_DOUBLE((double)i, lept_get_number(lept_get_array_element(&a, i)));
    }

    lept_popback_array_element(&a);
    EXPECT_EQ_SIZE_T(9, lept_get_array_size(&a));
    for (i = 0; i < 9; i++)
        EXPECT_EQ_DOUBLE((double)i, lept_get_number(lept_get_array_element(&a, i)));

    lept_erase_array_element(&a, 4, 0);
    EXPECT_EQ_SIZE_T(9, lept_get_array_size(&a));
    for (i = 0; i < 9; i++)
        EXPECT_EQ_DOUBLE((double)i, lept_get_number(lept_get_array_element(&a, i)));

    lept_erase_array_element(&a, 8, 1);
    EXPECT_EQ_SIZE_T(8, lept_get_array_size(&a));
    for (i = 0; i < 8; i++)
        EXPECT_EQ_DOUBLE((double)i, lept_get_number(lept_get_array_element(&a, i)));

    lept_erase_array_element(&a, 0, 2);
    EXPECT_EQ_SIZE_T(6, lept_get_array_size(&a));
    for (i = 0; i < 6; i++)
        EXPECT_EQ_DOUBLE((double)i + 2, lept_get_number(lept_get_array_element(&a, i)));

    lept_free(&a1);
    lept_free(&a2);
    lept_free(&a);
    lept_free(&e);
}


/**
 * @brief 测试对象类型相关接口
 * 
 */
static void test_access_object() {
    lept_value o, v, *pv;
    size_t i, j, index;

    lept_init(&o);

    for (j = 0; j <= 5; j++) {
        lept_set_object(&o, j);
        EXPECT_EQ_SIZE_T(0, lept_get_object_size(&o));
        EXPECT_EQ_SIZE_T(j, lept_get_object_capacity(&o));
        for (i = 0; i < 10; i++) {
            char key[2] = "a";
            key[0] += i;
            lept_init(&v);
            lept_set_number(&v, i);
            lept_move(lept_set_object_value(&o, key, 1), &v);
            lept_free(&v);
        }
        EXPECT_EQ_SIZE_T(10, lept_get_object_size(&o));
        for (i = 0; i < 10; i++) {
            char key[] = "a";
            key[0] += i;
            index = lept_find_object_index(&o, key, 1);
            EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
            pv = lept_get_object_value(&o, index);
            EXPECT_EQ_DOUBLE((double)i, lept_get_number(pv));
        }
    }

    index = lept_find_object_index(&o, "j", 1); // index = 9   
    EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
    lept_remove_object_value_index(&o, index);  // remove "j":9
    index = lept_find_object_index(&o, "j", 1);
    EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
    EXPECT_EQ_SIZE_T(9, lept_get_object_size(&o));

    index = lept_find_object_index(&o, "a", 1);  // index = 0
    EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
    lept_remove_object_value_index(&o, index);   // remove "a":1
    index = lept_find_object_index(&o, "a", 1);
    EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
    EXPECT_EQ_SIZE_T(8, lept_get_object_size(&o));

    EXPECT_TRUE(lept_get_object_capacity(&o) > 8);
    lept_shrink_object(&o);
    EXPECT_EQ_SIZE_T(8, lept_get_object_capacity(&o));
    EXPECT_EQ_SIZE_T(8, lept_get_object_size(&o));
    for (i = 0; i < 8; i++) {
        char key[] = "a";
        key[0] += i + 1;
        EXPECT_EQ_DOUBLE((double)i + 1, 
        lept_get_number(lept_get_object_value(&o, lept_find_object_index(&o, key, 1))));
    }

    lept_set_string(&v, "Hello", 5);
    lept_move(lept_set_object_value(&o, "World", 5), &v); /* Test if element is freed */
    lept_free(&v);

    pv = lept_find_object_value(&o, "World", 5);
    EXPECT_TRUE(pv != NULL);
    EXPECT_EQ_STRING("Hello", lept_get_string(pv), lept_get_string_length(pv));

    i = lept_get_object_capacity(&o);
    lept_clear_object(&o);
    EXPECT_EQ_SIZE_T(0, lept_get_object_size(&o));
    EXPECT_EQ_SIZE_T(i, lept_get_object_capacity(&o)); /* capacity remains unchanged */
    lept_shrink_object(&o);
    EXPECT_EQ_SIZE_T(0, lept_get_object_capacity(&o));

    lept_free(&o);
}


/**
 * @brief: 测试把 JSON 值转换为数字类型字符串
 * @notes: 这里用到了往返测试，即先将原始字符串解析为JSON值，再stringfy为目标字符串，比较两个字符串是否一致 
 */
static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}


/**
 * @brief 测试把 JSON 值转换为字符串类型字符串
 * 
 */
static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}


/**
 * @brief 测试把 JSON 值转换为数组类型字符串
 * 
 */
static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[\"abc\"]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}


/**
 * @brief 测试把 JSON 值转换为对象类型字符串
 * 
 */
static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}


/**
 * @brief JSON文本生成器集成测试
 * 
 */
static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}


/**
 * @brief API函数测试：lept_is_equal()
 * 
 */
static void test_is_euqal() {
    TEST_EQUAL("true", "true", 1);
    TEST_EQUAL("true", "false", 0);
    TEST_EQUAL("false", "false", 1);
    TEST_EQUAL("null", "null", 1);
    TEST_EQUAL("null", "0", 0);
    TEST_EQUAL("123", "123", 1);
    TEST_EQUAL("123", "456", 0);
    TEST_EQUAL("\"abc\"", "\"abc\"", 1);
    TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
    TEST_EQUAL("[]", "[]", 1);
    TEST_EQUAL("[]", "null", 0);
    TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
    TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
    TEST_EQUAL("[[]]", "[[]]", 1);
    TEST_EQUAL("{}", "{}", 1);
    TEST_EQUAL("{}", "null", 0);
    TEST_EQUAL("{}", "[]", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
    TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
    TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}


/**
 * @brief API函数测试：lept_copy()
 * 
 */
static void test_copy() {
    lept_value v1, v2;
    lept_init(&v1);
    lept_parse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    lept_init(&v2);
    lept_copy(&v2, &v1);
    EXPECT_TRUE(lept_is_equal(&v2, &v1));
    lept_free(&v1);
    lept_free(&v2);
}


/**
 * @brief API函数测试：lept_move()
 * 
 */
static void test_move() {
    lept_value v1, v2, v3;
    lept_init(&v1);
    lept_parse(&v1, "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
    lept_init(&v2);
    lept_copy(&v2, &v1);
    lept_init(&v3);
    lept_move(&v3, &v2);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v2));
    EXPECT_TRUE(lept_is_equal(&v3, &v1));
    lept_free(&v1);
    lept_free(&v2);
    lept_free(&v3);
}


/**
 * @brief API函数测试：lept_swap()
 * 
 */
static void test_swap() {
    lept_value v1, v2;
    lept_init(&v1);
    lept_init(&v2);
    lept_set_string(&v1, "Hello",  5);
    lept_set_string(&v2, "World!", 6);
    lept_swap(&v1, &v2);
    EXPECT_EQ_STRING("World!", lept_get_string(&v1), lept_get_string_length(&v1));
    EXPECT_EQ_STRING("Hello",  lept_get_string(&v2), lept_get_string_length(&v2));
    lept_free(&v1);
    lept_free(&v2);
}


/**
 * @brief API函数继承测试：拷贝、移动、交换
 * 
 */
static void test_copy_move_swap() {
    const char* json = "{\"a\":[1,2],\"b\":3}";
    char* json2;
    size_t length;
    lept_value v;
    lept_init(&v);

    lept_parse(&v, json);
    lept_copy(lept_find_object_value(&v, "b", 1), lept_find_object_value(&v, "a", 1));
    lept_stringify(&v, &json2, &length);
    EXPECT_EQ_STRING("{\"a\":[1,2],\"b\":[1,2]}", json2, length);
    free(json2);
    lept_free(&v);

    lept_parse(&v, json);
    lept_move(lept_find_object_value(&v, "b", 1), lept_find_object_value(&v, "a", 1));
    lept_stringify(&v, &json2, &length);
    EXPECT_EQ_STRING("{\"a\":null,\"b\":[1,2]}", json2, length);
    free(json2);
    lept_free(&v);

    lept_parse(&v, json);
    lept_swap(lept_find_object_value(&v, "a", 1), lept_find_object_value(&v, "b", 1));
    lept_stringify(&v, &json2, &length);
    EXPECT_EQ_STRING("{\"a\":3,\"b\":[1,2]}", json2, length);
    free(json2);
    lept_free(&v);
}


int main() {
    // 测试解析器
    test_parse_literal();
    test_parse_number();
    test_parse_invalid_num();
    test_parse_string();
    test_parse_invalid_str();
    test_parse_invalid_unicode();
    test_parse_array();
    test_parse_invalid_array();
    test_parse_object();
    test_parse_invalid_obj();

    // 测试access接口
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
    test_access_arrary();
    test_access_object();

    // 测试生成器
    test_stringify();

    // 测试基础函数
    test_is_euqal();
    test_copy();
    test_move();
    test_swap();
    test_copy_move_swap();

    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
