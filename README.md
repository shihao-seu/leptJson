# leptJson开发日志
本库参考/miloyip/json-tutorial，通过一个toy project巩固自己的C语言基础，并且初步了解一个简单的项目开发形式。

## 需求与功能

* JSON（JavaScript Object Notation）是一个用于数据交换的文本格式，C程序需要解析JSON文本、转换成内部结构体，同时也能将结构体转换为JSON文本，达到双向转换的目的。即：
  1. 把 JSON 文本解析为一个树状数据结构（parse）
  2. 提供接口访问该数据结构（access）
  3. 把数据结构转换成 JSON 文本（stringify）

## 项目特色

一个轻量的JSON库（名称取自标准模型中的轻子lepton），包含以下特性：

* 符合标准的 JSON 解析器和生成器

* 手写的递归下降解析器（recursive descent parser）

* 使用标准 C 语言（C89）

* 仅支持 UTF-8 JSON 文本

* 仅支持以 `double` 存储 JSON number 类型

* 代码量：

```
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                                2            140             92           1365
C/C++ Header                     1             94            281             85
-------------------------------------------------------------------------------
SUM:                             3            234            373           1450
-------------------------------------------------------------------------------
```

## 文件说明

* `leptjson.h`：leptjson 的头文件（header file），含有对外的类型和 API 函数声明。
* `leptjson.c`：leptjson 的实现文件（implementation file），含有内部的类型声明和函数实现。此文件会编译成库。
* `test.c`：我们使用测试驱动开发（test driven development, TDD）。此文件包含测试程序，需要链接 leptjson 库。

## API设计

1. JSON 中有 6 种数据类型，如果把 true 和 false 当作两个类型就是 7 种，我们为此声明一个枚举类型（enumeration type）：

   ```c
   typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;
   ```

2. 声明 JSON 的数据结构。JSON 是一个树形结构，我们最终需要实现一个树的数据结构，每个节点使用 `lept_value` 结构体表示，我们会称它为一个 JSON 值（JSON value）：

   ```c
   typedef struct t_lept_value{
       union {
           struct { lept_member* m; size_t size, capacity; }o; /* obeject*/
           struct { char* str; size_t size; }s;                /* string */
           struct { lept_value* e; size_t size, capacity; }a;  /* array  */
           double n;                                           /* number */
       } u;
       lept_type type;
   } lept_value;
   ```

3. 把 JSON 文本解析为一个树状数据结构（parse），即JSON值

   ```c
   /**
    * @brief 解析C风格字符串，转换成JSON值
    * 
    * @param [out] v: 程序可读结构体
    * @param [in] json: 字符串指针
    * @return int : 解析结果
    */
   int lept_parse(lept_value* v, const char* json);
   ```

4. 提供接口访问该数据结构（access），比如：

   ```c
   /**
    * @brief 获取json值类型
    * 
    * @param v 
    * @return lept_type 
    */
   lept_type lept_get_type(const lept_value* v);
   
   /**
    * @brief 获取数值
    * 
    * @param v 
    * @return double 
    */
   double lept_get_number(const lept_value* v);
   
   /**
    * @brief 设置数组初始容量
    * 
    * @param v 
    * @param capacity 
    */
   void lept_set_array(lept_value* v, size_t capacity);
   
   /**
    * @brief 数组尾后添加新元素
    * 
    * @param v 
    * @return lept_value* 
    */
   lept_value* lept_pushback_array_element(lept_value* v);
   
   /**
    * @brief 按key寻找obj的value
    * 
    * @param v 
    * @param key 
    * @param klen 
    * @return lept_value* 
    */
   lept_value* lept_find_object_value(const lept_value* v, const char* key, size_t klen);
   ```

5. 把数据结构转换成 JSON 文本（stringify）

   ```c
   /**
    * @brief 生成JSON字符串
    * 
    * @param [in] v: json值
    * @param [out] json: 输出C字符串 
    * @param [out] length: 字符串长度 
    * @return int: 调用结果
    */
   int lept_stringify(const lept_value* v, char** json, size_t* length);
   ```
## 测试驱动开发

* 一般来说，软件开发是以周期进行的。例如，加入一个功能，再写关于该功能的单元测试。但也有另一种软件开发方法论，称为测试驱动开发（test-driven development, TDD），它的主要循环步骤是：

  1. 加入一个测试。
  2. 运行所有测试，新的测试应该会失败。
  3. 编写实现代码。
  4. 运行所有测试，若有测试失败回到3。
  5. 重构代码。
  6. 回到 1。
* TDD 是先写测试，再实现功能。好处是实现只会刚好满足测试，而不会写了一些不需要的代码，或是没有被测试的代码。
* 在本项目中，test.c 包含了一个极简的单元测试框架：
  
   ```c
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
   ```
* 单元测试项目包括：
  ```c
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
  ```
* 看一下最终的测试结果：
  ```
  cmake -B ./debug -DCMAKE_BUILD_TYPE=Debug && ./debug/leptjson_test
   -- Configuring done
   -- Generating done
   -- Build files have been written to: /leptJson/debug
   741/741 (100.00%) passed
  ```
