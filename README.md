# grpc-cxx-demo

## 基本信息

本 Demo 是基于官方示例 route_guide 进行调整，增加了基于拦截器(interceptor)实现的RPC接口调用日志打印功能， 在每次 RPC 调用时自动打印出 RPC 服务名、请求参数、返回参数、返回结果状态，方便查看接口调用情况。目前只是打印到控制台，可以再引入一个日志框架，将这些信息输出到日志文件中。

## 文件说明

* log_interceptor_server.h: 服务端拦截器实现
* log_interceptor_client.h: 客户端拦截器实现

## 编译说明

建议使用 cmake 进行编译。参考步骤：

```bash
mkdir -p cmake/build
cd cmake/build
cmake ../..
make -j
```

## 参考资料

* [grpc::experimental::InterceptorBatchMethods](https://grpc.github.io/grpc/cpp/classgrpc_1_1experimental_1_1_interceptor_batch_methods.html)
* 客户端拦截器实现可参考官方示例[keyvaluestore]
* 服务端拦截器实现可参考[LoggingInterceptor](https://github.com/grpc/grpc/blob/master/test/cpp/end2end/server_interceptors_end2end_test.cc)
