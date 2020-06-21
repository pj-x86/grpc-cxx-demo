#ifndef _LOG_INTERCEPTOR_SERVER_H_
#define _LOG_INTERCEPTOR_SERVER_H_
/*
 * 服务端日志拦截器实现
 * 日期: 2020-06-20
 * 作者: pj
 */

#include <map>

#include <grpcpp/support/server_interceptor.h>
#include <google/protobuf/util/json_util.h>

#include "route_guide.grpc.pb.h"

// 
class ServerLoggingInterceptor : public grpc::experimental::Interceptor {
 public:
  ServerLoggingInterceptor(grpc::experimental::ServerRpcInfo* info) {
      info_ = info;
  }

  void Intercept(grpc::experimental::InterceptorBatchMethods* methods) override {
    bool hijack = false;
    if (methods->QueryInterceptionHookPoint(
            grpc::experimental::InterceptionHookPoints::
                PRE_SEND_INITIAL_METADATA)) {
      // Hijack all calls
      //hijack = true;
    }
    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) { //服务端发送返回消息之前
      std::cout << "---InterceptionHookPoints::PRE_SEND_MESSAGE---" << std::endl;

      const grpc::protobuf::Message* req_msg = nullptr;
      std::string req_msg_str;

      req_msg = static_cast<const grpc::protobuf::Message*>(methods->GetSendMessage());
      if (req_msg == nullptr) { //某些场景下非序列化消息不可用，此时需要再取一次序列化的消息再做类型转换
        // 此分支一般不会进入，目前没遇到过，是否需要有待验证
        std::cout << "未取到非序列化消息，再取一次序列化消息" << std::endl;
        auto* buffer = methods->GetSerializedSendMessage();
        auto copied_buffer = *buffer;

        if (strcmp(info_->method(), "/routeguide.RouteGuide/GetFeature") == 0
          || strcmp(info_->method(), "/routeguide.RouteGuide/ListFeatures") == 0){
          req_msg_feature.Clear();
          GPR_ASSERT(
              grpc::SerializationTraits<routeguide::Feature>::Deserialize(&copied_buffer, &req_msg_feature)
                  .ok());
          req_msg = &req_msg_feature; 
        }
        else if (strcmp(info_->method(), "/routeguide.RouteGuide/RecordRoute") == 0){
          req_msg_summary.Clear();
          GPR_ASSERT(
              grpc::SerializationTraits<routeguide::RouteSummary>::Deserialize(&copied_buffer, &req_msg_summary)
                  .ok());
          req_msg = &req_msg_summary; 
        }
        else if (strcmp(info_->method(), "/routeguide.RouteGuide/RouteChat") == 0){
          req_msg_route.Clear();
          GPR_ASSERT(
              grpc::SerializationTraits<routeguide::RouteNote>::Deserialize(&copied_buffer, &req_msg_route)
                  .ok());
          req_msg = &req_msg_route; 
        }
      }
      
      std::cout << "RPC接口: "<< info_->method() << ", ";
      if (req_msg != nullptr){
        std::cout << "返回参数: " << req_msg->GetTypeName() << std::endl;
        // 输出简洁模式
        //std::cout << req_msg->DebugString() << std::endl;
        //std::cout << req_msg->ShortDebugString() << std::endl;

        // 将 Message 转换为 JSON 字符串，方便阅读
        google::protobuf::util::MessageToJsonString(*req_msg, &req_msg_str);
        std::cout << req_msg_str << std::endl;
      } else {
        std::cout << "获取返回参数失败或返回结束." << std::endl;
      }
    }

    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) { //服务端收到请求消息之后
      std::cout << "---InterceptionHookPoints::POST_RECV_MESSAGE---" << std::endl;

      //routeguide::Feature* resp = static_cast<routeguide::Feature*>(methods->GetRecvMessage());
      grpc::protobuf::Message* resp = static_cast<grpc::protobuf::Message*>(methods->GetRecvMessage());
      std::string resp_str;

      if (resp != nullptr) {
        std::cout << "RPC接口: "<< info_->method() << ", ";
        std::cout << "请求参数: " << resp->GetTypeName() << std::endl;
        // 将 Message 转换为 JSON 字符串，方便阅读
        google::protobuf::util::MessageToJsonString(*resp, &resp_str);
        std::cout << resp_str << std::endl;
      } else {
        // 对于流式 RPC 调用结束时，会进入此分支
        std::cout << "RPC接口: "<< info_->method() << ", 接收请求消息结束." << std::endl;
      }
      
    }

    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_STATUS)) {
      std::cout << "---InterceptionHookPoints::PRE_SEND_STATUS---" << std::endl;

      grpc::Status status = methods->GetSendStatus();
      std::string resp_msg = status.ok()?"成功":status.error_message();
      std::cout << "RPC接口: "<< info_->method() << ", ";
      std::cout << "返回值: " << resp_msg << std::endl;
    }

    // One of Hijack or Proceed always needs to be called to make progress.
    if (hijack) {
      // Hijack is called only once when PRE_SEND_INITIAL_METADATA is present in
      // the hook points
      methods->Hijack();
    } else {
      // Proceed is an indicator that the interceptor is done intercepting the
      // batch.
      methods->Proceed();
    }
  }

private:
  grpc::experimental::ServerRpcInfo* info_;
  routeguide::Feature req_msg_feature;
  routeguide::RouteSummary req_msg_summary;
  routeguide::RouteNote req_msg_route;
};

class ServerLoggingInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
 public:
  grpc::experimental::Interceptor* CreateServerInterceptor(
      grpc::experimental::ServerRpcInfo* info) override {
    return new ServerLoggingInterceptor(info);
  }
};

#endif //_LOG_INTERCEPTOR_SERVER_H_
