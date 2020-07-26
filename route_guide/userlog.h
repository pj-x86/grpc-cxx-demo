#ifndef USER_LOG_H_
#define USER_LOG_H_

// 在 spdlog.h 之前定义以下宏可以在编译时控制 SPDLOG_ 系列宏是否启用
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"

// 初始化日志框架
// env: 环境标识，只有 dev 开发环境会同时向控制台和日志文件输出，可选值有 {"dev", "test", "prod"}
// logfile_prefix: 日志文件前缀，包含路径，格式如 logs/server，生成的日志文件名类似 logs/server-2020-07-25.log
// log_level: 日志文件日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
void init_logger(const std::string &env,const std::string &logfile_prefix, const std::string &log_level);
// 退出日志框架，释放资源
void exit_logger();

// 动态修改全局日志级别
// log_level: 日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
void modify_log_level(const std::string &log_level);

#endif //USER_LOG_H_