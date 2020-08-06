/**
 * @file route_guide_server.cc
 * @author pj-x86 (pj81102@163.com)
 * @brief 服务端主程序入口
 * @version 0.1
 * @date 2020-08-05
 * 
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <unistd.h>
#include <signal.h>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "userlog.h"
#include "helper.h"
#include "log_interceptor_server.h"
#include "SimpleIni.h" //配置文件读写工具类

#include "route_guide.grpc.pb.h"
#include "route_guide.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using routeguide::Feature;
using routeguide::Point;
using routeguide::Rectangle;
using routeguide::RouteGuide;
using routeguide::RouteNote;
using routeguide::RouteSummary;
using std::chrono::system_clock;

constexpr char configFile[] = "./config.ini";

static CSimpleIniA gSimpleIni;

/**
 * @brief 配置信息
 * 
 */
typedef struct STConfigInfo
{
    std::string LogPath;
    std::string LogLevel;
    std::string Env;

    std::string LiqDBUser;
    std::string LiqDBPasswd;
    std::string LiqDBName;

    std::string ServerPort;

    std::string FileDBPath;
} STConfigInfo;

static STConfigInfo gConfigInfo;

/**
 * @brief 解析命令行参数
 * 
 * @param sArg 命令行原始参数，如 --port=20202 或 --port 20202
 * @param sKey 参数名部分，如 --port
 * @param sVal 输出参数值，如 20202
 * @return int 返回值 0 表示成功，-1表示失败
 */
static int ParseArg(const char *sArg, const std::string &sKey, std::string &sVal)
{
    std::string argv = sArg;

    size_t start_position = argv.find(sKey);
    if (start_position != std::string::npos)
    {
        start_position += sKey.size();
        if (argv[start_position] == ' ' || argv[start_position] == '=')
        {
            sVal = argv.substr(start_position + 1);
        }
        else
            return -1;
    }
    else
        return -1;

    return 0;
}

/**
 * @brief 读取配置文件
 * 
 * @return int 返回值 0 表示成功，-1表示失败
 */
static int ReadConfigFile()
{
    gSimpleIni.SetUnicode();
    SI_Error rc = gSimpleIni.LoadFile(configFile);
    if (rc < 0)
    {
        std::cerr << "加载配置文件[" << configFile << "]失败" << std::endl;
        return -1;
    }

    // 读取配置参数
    const char *pv;
    pv = gSimpleIni.GetValue("log", "path", "../log/server");
    gConfigInfo.LogPath = pv;
    std::cout << "日志路径=" << gConfigInfo.LogPath << std::endl;

    pv = gSimpleIni.GetValue("log", "level", "info");
    gConfigInfo.LogLevel = pv;
    std::cout << "日志级别=" << gConfigInfo.LogLevel << std::endl;

    pv = gSimpleIni.GetValue("log", "env", "dev");
    gConfigInfo.Env = pv;
    std::cout << "当前环境=" << gConfigInfo.Env << std::endl;

    return 0;
}

/**
 * @brief 处理 SIGUSR1 信号的信号处理回调函数
 * 
 * @param signum 信号
 */
static void HandleSignal(int signum)
{
    //std::cout << "收到信号: " << signum << std::endl;
    SPDLOG_INFO("收到信号: {:d}", signum);

    // 重新读取配置文件中的日志级别
    SI_Error rc = gSimpleIni.LoadFile(configFile);
    if (rc < 0)
    {
        //std::cerr << "加载配置文件[" << configFile << "]失败" << std::endl;
        SPDLOG_INFO("重新加载配置文件: {} 失败", configFile);
        return;
    }
    
    // 读取配置参数
    const char *pv;
    pv = gSimpleIni.GetValue("log", "level", "info");
    gConfigInfo.LogLevel = pv;

    //动态修改日志级别
    modify_log_level(gConfigInfo.LogLevel);
}

/**
 * @brief 启动 gRPC 服务器
 * 
 * @param server_port 服务监控端口
 * @param db_path 地理位置信息文件数据库
 */
void RunServer(const std::string &server_port, const std::string &db_path)
{
    std::string server_address("0.0.0.0:"+server_port);
    routeguide::RouteGuideImpl service(db_path);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    // 创建服务端拦截器
    std::vector<
        std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>>
        interceptor_creators;
    interceptor_creators.push_back(
        std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>(
            new ServerLoggingInterceptorFactory()));
    builder.experimental().SetInterceptorCreators(std::move(interceptor_creators));

    std::unique_ptr<Server> server(builder.BuildAndStart());
    //std::cout << "Server listening on " << server_address << std::endl;
    SPDLOG_INFO("服务启动成功，监听端口为 {}", server_address);

    server->Wait();
}

int main(int argc, char **argv)
{
    int iRet = 0;

    // 命令行选项解析
    if (argc < 3)
    {
        std::cout << "启动格式示例: " << argv[0] << " --port=20202 --db_path=./route_guide_db.json" << std::endl;
        exit(-1);
    }

    iRet = ParseArg(argv[1], "--port", gConfigInfo.ServerPort);
    if (iRet < 0)
    {
        std::cout << "请设置服务端口: --port=xxx" << std::endl;
        exit(-1);
    }
    iRet = ParseArg(argv[2], "--db_path", gConfigInfo.FileDBPath);
    if (iRet < 0)
    {
        std::cout << "请设置地理信息文件数据库: --db_path=xxx.json" << std::endl;
        exit(-1);
    }

    // 读取配置文件
    iRet = ReadConfigFile();
    if (iRet < 0)
    {
        exit(-1);
    }

    // 初始化日志框架
    init_logger(gConfigInfo.Env, gConfigInfo.LogPath+"_"+gConfigInfo.ServerPort, gConfigInfo.LogLevel);

    //设置信号处理函数，专门处理 SIGUSR1，用于重新读取配置文件日志级别
    struct sigaction sa;
    sa.sa_handler = HandleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        /* Handle error */
        SPDLOG_ERROR("设置信号处理函数发生异常");
        exit(-1);
    }
        
    //初始化数据库连接池
    //TODO

    std::string db = routeguide::GetDbFileContent(gConfigInfo.FileDBPath);

    //启动服务
    RunServer(gConfigInfo.ServerPort, db);

    //退出日志框架
    exit_logger();

    return 0;
}
