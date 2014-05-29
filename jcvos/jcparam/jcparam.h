#pragma once

// jcparam模块主要提供一些通用的参数服务。
// 参数的基本形式由“参数名称”(Key)和“参数值”(Value)构成。Value是一个通用的值类型，提供一个通用的界面。
// Value可以是一个简单类型、自定义类型、或者复核类型。复核类型包括以名称索引的set和以下标索引的array。
// Value同时提供一套从制转换到字符串的方法。转换的方法基本按照xml语法。
#include "include/ivalue.h"
#include "include/value.h"
#include "include/param_define.h"
#include "include/string_table.h"
#include "include/table_base.h"
#include "include/jcstream.h"
#include "include/general_table.h"
//#include "include/dynamic_table.h"