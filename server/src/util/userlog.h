/**
 * @file userlog.h
 * @author pj-x86 (pj81102@163.com)
 * @brief spdlog日志框架初始化、退出函数
 * @version 0.1
 * @date 2020-08-05
 * 
 */

#ifndef _USER_LOG_H_
#define _USER_LOG_H_

// 在 spdlog.h 之前定义以下宏可以在编译时控制 SPDLOG_ 系列宏是否启用
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"

/**
 * @brief 初始化日志框架
 * 
 * @param env 环境标识，只有 dev 开发环境会同时向控制台和日志文件输出，可选值有 {"dev", "test", "prod"}
 * @param logfile_prefix 日志文件前缀，包含路径，格式如 logs/server，生成的日志文件名类似 logs/server-2020-07-25.log
 * @param log_level 日志文件日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
 */
void init_logger(const std::string &env,const std::string &logfile_prefix, const std::string &log_level);

/**
 * @brief 退出日志框架，释放资源
 * 
 */
void exit_logger();

/**
 * @brief 动态修改全局日志级别
 * 
 * @param log_level 日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
 */
void modify_log_level(const std::string &log_level);

#endif //_USER_LOG_H_