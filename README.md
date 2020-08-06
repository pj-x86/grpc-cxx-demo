# grpc-cxx-demo

## 基本信息

本 Demo 基于官方示例 route_guide 进行改造，增加了以下功能：
* 基于拦截器(interceptor)实现的 RPC 接口调用日志打印功能， 在每次 RPC 调用时自动打印出 RPC 服务名、请求参数、返回参数、返回结果状态，方便查看接口调用情况。
* 引入 spdlog 日志框架，支持打印日志信息到控制台和日志文件。同时支持向进程发送信号动态修改日志级别。
* 增加读取配置文件 config.ini

## 文件说明

* log_interceptor_server.h: 服务端拦截器实现
* log_interceptor_client.h: 客户端拦截器实现
* userlog.cc: 引入开源 spdlog 日志库
* SimpleIni.h: 第三方开源INI配置文件读写库

## 编译说明

使用 cmake 进行编译。参考步骤：

```bash
cd server
mkdir -p cmake/build
cd cmake/build
cmake ../..
make -j
```

## 依赖说明

* 安装 gRPC(>=1.30.1) 和 protobuf(>=3.12.2.0)
* 本代码库引入了开源 spdlog(>=1.6.1) 作为底层日志库，详见 CMakeLists.txt 。 spdlog 官方推荐安装静态库版本，安装详见[spdlog](https://github.com/gabime/spdlog)。

## 参考资料

* [grpc::experimental::InterceptorBatchMethods](https://grpc.github.io/grpc/cpp/classgrpc_1_1experimental_1_1_interceptor_batch_methods.html)
* 客户端拦截器实现可参考官方示例[keyvaluestore]
* 服务端拦截器实现可参考[LoggingInterceptor](https://github.com/grpc/grpc/blob/master/test/cpp/end2end/server_interceptors_end2end_test.cc)
