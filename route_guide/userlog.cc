// spdlog 日志框架初始化操作

#include "userlog.h"

#include "spdlog/cfg/env.h" // for loading levels from the environment variable
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/cfg/env.h"

// static void load_levels_example()
// {
//     // Set the log level to "info" and mylogger to "trace":
//     // SPDLOG_LEVEL=info,mylogger=trace && ./example
//     spdlog::cfg::load_env_levels();
//     // or from command line:
//     // ./example SPDLOG_LEVEL=info,mylogger=trace
//     // #include "spdlog/cfg/argv.h" // for loading levels from argv
//     // spdlog::cfg::load_argv_levels(args, argv);
// }

// A logger with multiple sinks (stdout and file) - each with a different format and log level.
static void create_multi_sink(const std::string &env,const std::string &logfile_prefix, const std::string &log_level)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    //console_sink->set_pattern("%Y-%m-%d %H:%M:%S.%e|%P|%t|%l|create_multi_sink|%v");

    // auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    // file_sink->set_level(spdlog::level::trace);
    // file_sink->set_pattern("%Y-%m-%d %H:%M:%S.%e|%P|%t|%l|create_multi_sink|%v");

    // Create a daily logger - a new file is created every day on 0:00am.
    auto daily_file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logfile_prefix+".log", 0, 0);
    daily_file_sink->set_level(spdlog::level::from_str(log_level));

    //spdlog::logger logger("multi_sink", {console_sink, daily_file_sink});
    spdlog::sinks_init_list sink_list;
    if (env == "dev")
        sink_list = {daily_file_sink, console_sink};
    else
        sink_list = {daily_file_sink};
    auto multi_logger = std::make_shared<spdlog::logger>("multi_sink", sink_list);

    spdlog::set_default_logger(multi_logger);

    spdlog::set_level(spdlog::level::from_str(log_level)); // Set global log level to info

    //spdlog::warn("this should appear in both console and file");
    //spdlog::info("this message should not appear in the console, only in the file");
}

// 初始化日志框架
// env: 环境标识，只有 dev 开发环境会同时向控制台和日志文件输出，可选值有 {"dev", "test", "prod"}
// logfile_prefix: 日志文件前缀，包含路径，格式如 logs/server，生成的日志文件名类似 logs/server-2020-07-25.log
// log_level: 日志文件日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
void init_logger(const std::string &env,const std::string &logfile_prefix, const std::string &log_level)
{
    try
    {
        // 同时向控制台和文件打印日志
        create_multi_sink(env, logfile_prefix, log_level);

        // 自定义日志输出格式，包含自定义格式化参数 %*
        //custom_flags_example();

        // 定制日志输出格式
        // Customize msg format for all loggers
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e|%P-%t|%l|%s:%#|%v");

        // Log levels can be loaded from argv/env using "SPDLOG_LEVEL"
        //load_levels_example();

        // Flush all *registered* loggers using a worker thread every 3 seconds.
        // note: registered loggers *must* be thread safe for this to work correctly!
        spdlog::flush_every(std::chrono::seconds(3));
    }
    // Exceptions will only be thrown upon failed logger or sink construction (not during logging).
    catch (const spdlog::spdlog_ex &ex)
    {
        std::printf("Log initialization failed: %s\n", ex.what());
        exit(-1);
    }
    catch(const std::exception& ex)
    {
        std::printf("unknown exception: %s\n", ex.what());
        exit(-1);
    }

    //spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    SPDLOG_INFO("启动 spdlog 日志框架成功，spdlog 当前版本为 {}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    //spdlog::info("当前文件日志级别为 {}", spdlog::level::to_string_view(spdlog::default_logger()->level()));
    SPDLOG_INFO("当前文件日志级别为 {}", spdlog::level::to_string_view(spdlog::default_logger()->level()));
}

// 退出日志框架，释放资源
void exit_logger()
{
    // Release all spdlog resources, and drop all loggers in the registry.
    // This is optional (only mandatory if using windows + async log).
    SPDLOG_INFO("退出 spdlog 日志框架");
    spdlog::shutdown();
}

// 动态修改全局日志级别
// log_level: 日志级别，可选值有 {"trace", "debug", "info", "warning", "error", "critical", "off"}
void modify_log_level(const std::string &log_level)
{
    SPDLOG_INFO("修改日志级别，修改前日志级别为 {}", spdlog::level::to_string_view(spdlog::default_logger()->level()));
    //spdlog::default_logger()->flush();
    spdlog::set_level(spdlog::level::from_str(log_level)); // Set global log level to info
    SPDLOG_INFO("修改日志级别，修改后日志级别为 {}", spdlog::level::to_string_view(spdlog::default_logger()->level()));
}