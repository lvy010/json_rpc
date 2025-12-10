/*
 *  1.日志宏
 *  2.Json序列化反序列化
 *  3.生成uuid
 */
#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <chrono>
#include <random>
#include <atomic>
#include <iomanip> // setw

#include <ctime>

#include <jsoncpp/json/json.h>

namespace JsonRpc
{
    #define LOG_DEBUG 0
    #define LOG_INFO 1
    #define LOG_ERROR 2
    #define LOG_LINE LOG_DEBUG

    // 日志宏
    #define LOG(level, format, ...) {\
        if (level >= LOG_LINE) /* 低于等级的日志不输出 */ \
        { \
            time_t t = time(nullptr); /* 获取时间戳 */ \
            struct tm* lt = localtime(&t); /* 转化为本地时间 */ \
            char time_buf[32] = {'\0'}; /* 字符串缓冲区 */ \
            strftime(time_buf, 31, "%m-%d %T", lt); /* 时间转为字符串 */ \
            fprintf(stdout, "[%s][%s:%d]: " format "\n", time_buf, __FILE__, __LINE__, ##__VA_ARGS__); /* 输出日志，不定参 */ \
        } \
    }

    #define D_LOG(format, ...) LOG(LOG_DEBUG, format,  ##__VA_ARGS__);
    #define I_LOG(format, ...) LOG(LOG_INFO, format,  ##__VA_ARGS__);
    #define E_LOG(format, ...) LOG(LOG_ERROR, format,  ##__VA_ARGS__);

    // Json 
    class JsonUtil
    {
    public:
        // 序列化
        static bool serialize(const Json::Value& val, std::string& body)
        {
            std::stringstream ss;
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
            int ret = sw->write(val, &ss);
            if (ret != 0)
            {
                E_LOG("json serialize error!");
                return false;
            }

            body = ss.str();
            return true;
        }

        // 反序列化
        static bool unSerialize(const std::string& body, Json::Value& val)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            std::string errs;
            bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), &val, &errs);
            if (!ret)
                E_LOG("json deserialize error: %s", errs.c_str());

            return ret;
        }
    };

    class UUID
    {
    public:
        static std::string uuid()
        {
            // uuid: 8字节随机数 + 8字节自增序号
            //       以 8-4-4-4-12 形式组织起来

            std::stringstream ss;

            // 机器随机数，通过硬件实现，随机性强，但是慢
            std::random_device rd; 

            // 以机器随机数为种子，构造伪随机数对象(此处不使用时间，防止短时间内多次生成随机数一样)
            std::mt19937 generator(rd());

            // 限定随机数范围，指定 0-255 也就是一次生成一个字节
            std::uniform_int_distribution<int> distribution(0, 255); 

            for (int i = 0; i < 8; i++)
            {
                if (i == 4 || i == 6)
                    ss << "-";
                //    限制两位宽度     不足的位置填充 0     转为十六进制   限定范围    生成随机数 
                ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
            }

            ss << "-";

            static std::atomic<size_t> seq(1); // 全局自增变量
            size_t cur = seq.fetch_add(1);

            for (int i = 7; i >=0 ; i--) // 从高位到低位拼接
            {
                if (i == 5)
                    ss << "-";
                //                                                 当前数字右移i*8位，按位与 0xff
                ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i*8)) & 0xFF);
            }

            return ss.str();
        }
    };
};

