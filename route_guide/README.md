# gRPC Basics: C++ sample code

The files in this folder are the samples used in [gRPC Basics: C++][],
a detailed tutorial for using gRPC in C++.

[gRPC Basics: C++]:https://grpc.io/docs/tutorials/basic/c.html

* 基于官方的 route_guide 示例，增加了客户端拦截器代码，在该拦截器中将每次 RPC 调用的服务名、请求消息、返回消息、调用结果打印到控制台，后续也可打印到日志文件中。
