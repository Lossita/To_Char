/*
    A program that realizes 'to_char' in ORACLE for Mysql.
    Copyright (C) 2022  Lossita

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mysql/mysql.h"  // IWYU pragma: keep
#include "mysql/mysql/udf_registration_types.h"
#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <regex>

extern "C" {
    bool to_char_init(UDF_INIT* initid, UDF_ARGS* args, char* message);
    void to_char_deinit(UDF_INIT* initid);
    char* to_char(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null, char *error);
}

char* to_char(char*);
char* to_char(double);
char* to_char(long long);
char* to_char(double, char*);
char* to_char(long long, char*, bool = true);
std::map<std::string, int> format_resolver(char*, int*, bool*);
void integer_function(int*, int, int, char*, int, char[], char[]);
void to_char(long long, char*, char*, char*, std::map<std::string, int>, int, int);

void to_char_deinit(UDF_INIT* initid){}

bool to_char_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if(args->arg_count < 1 || args->arg_count > 2)
    {
        strcpy(message,"This function only needs one or two parameters.");
        return 1;
    }
    if(args->arg_count == 2 && args->arg_type[1] != STRING_RESULT)
    {
        strcpy(message,"The second parameter needs STRING type.");
        return 1;
    }

    return 0;
}

char* to_char(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null, char *error)
{
    if(args->arg_count == 1)
    {
        try{
            if(args->arg_type[0] == STRING_RESULT)
            {
                auto param1 = static_cast<char*>(args->args[0]);
                result = to_char(param1);
            }
            else if(args->arg_type[0] == INT_RESULT)
            {
                auto param1 = *((long long*)(args->args[0]));
                result = to_char(param1);
            }
            else
            {
                auto t = static_cast<char*>(args->args[0]);
                double param1 = atof(t);
                result = to_char(param1);
            }
        }
        catch(std::exception e)
        {
            strcpy(error,"1");
            strcpy(result,"Find an error in one param function.");
        }
    }
    else
    {
        try{
            auto format = static_cast<char*>(args->args[1]);
            if(args->arg_type[0] == INT_RESULT)
            {
                auto param1 = *((long long*)(args->args[0]));
                result = to_char(param1,format);
            }
            else
            {
                auto t = static_cast<char*>(args->args[0]);
                double param1 = atof(t);
                result = to_char(param1,format);
            }
        }catch(std::exception e)
        {
            strcpy(error,"1");
            strcpy(result,"Find an error in two param function.");
        }
    }


    if(result == NULL || result == "")
    {
        strcpy(result,"hello");
        *length = 5;
    }
    else{
        *length = strlen(result);
    }
    return result;
}

char* to_char(char* input)
{
    return input;
}
char* to_char(double input)
{
    char* str = static_cast<char*>(malloc(2048));
    sprintf(str, "%lf", input);
    int index = 0;
    char* p = str;
    while(*(p++) != '\0')
        index++;

    for (int i = index - 1; i > 0; --i)
    {
        if (str[i] != '0')
            break;
        str[i] = '\0';
    }

    return str;
}
char* to_char(long long input)
{
    char* str = static_cast<char*>(malloc(2048));
    sprintf(str,"%lld",input);
    return str;
}
char* to_char(long long input, char* format, bool int_type)
{
    //save the string converted by long long
    char* str = static_cast<char*>(malloc(2048));

    //input length
    int input_len = 0;
    long long copy = input;
    while (copy != 0)
    {
        input_len++;
        copy /= 10;
    }

    //resolve format
    int len = 0;
    bool isOK = true;
    auto map = format_resolver(format, &len, &isOK);

    if (!isOK)
    {
        char* res = static_cast<char*>(malloc(2048));
        res[0] = 'E';
        res[1] = 'R';
        res[2] = 'R';
        res[3] = 'O';
        res[4] = 'R';
        res[5] = '\0';
        return res;
    }
    if (len < input_len)
    {
        char* res = static_cast<char*>(malloc(2048));
        int i = 0;
        for (; i < input_len; i++)
            res[i] = '#';

        res[i] = '\0';
        return res;
    }

    if(int_type)
    {
        if(map["."]){
        int format_dot = 0; // format的小数点位置
        while (true)
        {
            if (format[format_dot] == '.' || format[format_dot] == 'D' || format[format_dot] == 'd') // 有小数点
                break;
            if (format[format_dot] == '\0') // 没有小数点
                break;
            format_dot++;
        }
        format_dot += 1;
        int fc_dot = 0; //format的小数长度
        while (format[format_dot] >= '0' && format[format_dot++] <= '9')
            fc_dot++;

        input_len += fc_dot;
        while(fc_dot-- > 0)
            input *= 10;
        }
    }

    //convert long long to string
    sprintf(str,"%lld",input);

    char* res = static_cast<char*>(malloc(2048));
    to_char(input, str, res, format, map, len, input_len);
    return res;

}
char* to_char(double input, char* format)
{
    char* str = to_char(input);

    int input_dot = 0; // input的绝对值的小数位置
    while (str[input_dot] != '.')
        input_dot++;
    if (input < 0)
        input_dot--;
    free(str);

    // resolve format
    int len = 0; // format的数字个数
    bool isOK = true;
    auto map = format_resolver(format, &len, &isOK);

    char* res = static_cast<char*>(malloc(2048));

    int fmt_len = 0; // format整数部分长度
    int input_len = input_dot; // input整数部分长度
    for (int i = 0; format[i] != '\0'; i++)
    {
        if (format[i] == '0' || format[i] == '9')
            fmt_len++;
        if (format[i] == '.' || format[i] == 'D' || format[i] == 'd') // 有小数点
            break;
    }

    if (!isOK)
    {
        char* res = static_cast<char*>(malloc(2048));
        res[0] = 'E';
        res[1] = 'R';
        res[2] = 'R';
        res[3] = 'O';
        res[4] = 'R';
        res[5] = '\0';
        return res;
    }
    if (fmt_len < input_len)
    {
        char* res = static_cast<char*>(malloc(2048));
        int i = 0;
        for (; i < input_len; i++)
            res[i] = '#';

        res[i] = '\0';
        return res;
    }

    fmt_len++; // format的小数点位置
    while (true)
    {
        if (format[fmt_len] == '.' || format[fmt_len] == 'D' || format[fmt_len] == 'd') // 有小数点
            break;
        if (format[fmt_len] == '\0') // 没有小数点
            break;
        fmt_len++;
    }

    fmt_len++;
    int fc_dot = 0; //format的小数长度
    while (format[fmt_len] >= '0' && format[fmt_len++] <= '9')
        fc_dot++;

    input *= std::pow(10,fc_dot);
    //需要四舍五入
    if (std::abs(input - static_cast<long long>(input)) - 0.5 >= 0)
        input += input > 0? 1.0 : -1.0;

    res = to_char(static_cast<long long>(input), format, false);
    return res;
}
/// @brief 整数的转换
/// @param index res的index
/// @param len format的len
/// @param input_len 源len
/// @param p format
/// @param start 源起始位置
/// @param res 结果
/// @param str 源
void integer_function(int* index, int len, int input_len, char* p, int start, char res[], char str[])
{
    for (int i = len; *p != '\0'; p++)
    {
        if (*p == '9')
        {
            //add space
            if (i > input_len)
            {
                while(*p != '0' && i > input_len)
                {
                    *(p++) = ' ';
                    i--;
                }
                res[(*index)++] = ' ';
                i++;
                p--;
            }
            else {
                //copy number
                res[(*index)++] = str[start] != '\0' ? str[start] : '0';
                start++;
            }
            i--;
        }
        else if (*p == '0')
        {
            //add zero
            if (i > input_len)
            {
                if(i - 1 > input_len)
                    *(p+1) = '0';
                res[(*index)++] = '0';
            }
            else {
                //copy number
                res[(*index)++] = str[start] != '\0' ? str[start] : '0';
                start++;
            }
            i--;
        }
        // add ,
        else if (*p == ',')
            res[(*index)++] = ',';
        // add .
        else if (*p == '.' || *p == 'D' || *p == 'd')
            res[(*index)++] = '.';

        if (*p == 'M' || *p == 'm' || *p == 'P' || *p == 'p')
            break;
    }
}
/// @brief to_char核心
/// @param input 正负性
/// @param str 源
/// @param res 结果
/// @param format 
/// @param map format的map
/// @param len format数字个数
/// @param input_len 源长度
void to_char(long long input, char* str, char* res, char* format, std::map<std::string, int> map, int len, int input_len)
{
    char* p = format;
    int index = 0;
    int start = input < 0 ? 1 : 0;

    if (map["MI"]) {
        if (map["$"])
            res[index++] = '$';

        integer_function(&index, len, input_len, p, start, res, str);
        res[index++] = input < 0 ? '-' : '\0';
    }
    else if (map["PR"]) {
        if (map["$"])
            res[index++] = '$';

        if (input < 0)
        {
            res[index++] = '<';
            integer_function(&index, len, input_len, p, start, res, str);
            res[index++] = '>';
        }
        else
            integer_function(&index, len, input_len, p, start, res, str);
    }
    // if format is S
    else if (map["S"])
    {
        bool state = true;

        if (p[index] == 'S' || p[index] == 's')
        {
            // add positive or negative
            if (input < 0)
                res[index++] = '-';
            else
                res[index++] = '+';

            state = false;
        }

        if (map["$"])//add $ in the very beginning
            res[index++] = '$';

        integer_function(&index, len, input_len, p, start, res, str);

        if (state)
        {
            // add positive or negative
            if (input < 0)
                res[index++] = '-';
            else
                res[index++] = '+';
            state = false;
        }

    }
    else
    {
        if (input < 0)
            res[index++] = '-';

        if (map["$"])//add $ in the very beginning
            res[index++] = '$';

        integer_function(&index, len, input_len, p, start, res, str);
    }

    for(int i = 0; i < 10; i++)
    {
	    if(index > 2047)
            break;
        res[index++] = '\0';
    }

    int space_pos = 0;
    while(true)
    {
        if(res[space_pos] == '\0')
        {
            space_pos = -1;
            break;
        }
        if(res[space_pos] == ' ')
            break;
        space_pos++;
    }
    if(space_pos != -1)
    {
        while(space_pos -- > 0)
        {
            res[space_pos + 1] = res[space_pos];
            res[space_pos] = ' ';
        }
    }
}
/// @brief 解析format
/// @param format 
/// @param len [out] format数字个数
/// @param is_correct [out] format格式是否正确
/// @return format的map
std::map<std::string, int> format_resolver(char* format, int* len, bool* is_correct)
{
    std::map<std::string, int> map;
    map["MI"] = 0;
    map["PR"] = 0;
    map["S"] = 0;
    map["."] = 0;
    map["$"] = 0;
    char* p = format;

    // input的大致形式
    std::regex regex_str(R"(^[S|s]?[\$]?([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]{0,})(([MmPp]?[IiRr]?)|[S|s]?)$)");
    // input的MI形式
    std::regex mi(R"(^([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]{0,})[Mm][Ii]$)");
    // input的PR形式
    std::regex pr(R"(^([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]{0,})[Pp][Rr]$)");
    // input的前S形式
    std::regex frontS(R"(^[Ss]([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]{0,})$)");
    // input的后S形式
    std::regex backS(R"(^([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]{0,})[Ss]$)");
    // input的一般形式
    std::regex normal(R"(^([0|9|,|\$]{0,})[.|D|d]?([0|9|,|\$]){0,}$)");

    // 统计$和0，9的个数
    for (int i = 0; *p != '\0'; p++)
    {
        if (*p == '9' || *p == '0')
            (*len)++;
        if (*p == '$')
            map["$"]++;
        if(*p == 'D' || *p == 'd' || *p == '.')
            map["."]++;
    }

    if (std::regex_match(format, regex_str))
    {
        if (std::regex_match(format, mi))
            map["MI"] = 1;
        else if (std::regex_match(format, pr))
            map["PR"] = 1;
        else if (std::regex_match(format, frontS) || std::regex_match(format, backS))
            map["S"] = 1;
        else if(std::regex_match(format, normal))
            ;
        else
            *is_correct = false;
    }
    else
        *is_correct = false;

    if(map["$"] > 1)
        *is_correct = false;

    return map;
}