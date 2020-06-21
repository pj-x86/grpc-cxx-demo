#ifndef _LOG_INTERCEPTOR_CLIENT_H_
#define _LOG_INTERCEPTOR_CLIENT_H_
/*
 * 客户端日志拦截器实现
 * 日期: 2020-06-13
 * 作者: pj
 */

#include <map>

#include <grpcpp/support/client_interceptor.h>
#include <google/protobuf/util/json_util.h>

#include "route_guide.grpc.pb.h"

// 
class ClientLoggingInterceptor : public grpc::experimental::Interceptor {
 public:
  ClientLoggingInterceptor(grpc::experimental::ClientRpcInfo* info) {
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
    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) { //客户端发送请求消息之前
      std::cout << "---InterceptionHookPoints::PRE_SEND_MESSAGE---" << std::endl;

      const grpc::protobuf::Message* req_msg = nullptr;
      std::string req_msg_str;

      req_msg = static_cast<const grpc::protobuf::Message*>(methods->GetSendMessage());
      if (req_msg == nullptr) { //某些场景下非序列化消息不可用，此时需要再取一次序列化的消息再做类型转换
        // 此分支一般不会进入，目前没遇到过
        std::cout << "未取到非序列化消息，再取一次序列化消息" << std::endl;
        auto* buffer = methods->GetSerializedSendMessage();
        auto copied_buffer = *buffer;

        if (strcmp(info_->method(), "/routeguide.RouteGuide/GetFeature") == 0
          || strcmp(info_->method(), "/routeguide.RouteGuide/RecordRoute") == 0){
          req_msg_point.Clear();
          GPR_ASSERT(
              grpc::SerializationTraits<routeguide::Point>::Deserialize(&copied_buffer, &req_msg_point)
                  .ok());
          req_msg = &req_msg_point; 
        }
        else if (strcmp(info_->method(), "/routeguide.RouteGuide/ListFeatures") == 0){
          req_msg_rect.Clear();
          GPR_ASSERT(
              grpc::SerializationTraits<routeguide::Rectangle>::Deserialize(&copied_buffer, &req_msg_rect)
                  .ok());
          req_msg = &req_msg_rect; 
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
        std::cout << "请求参数: " << req_msg->GetTypeName() << std::endl;
        // 输出简洁模式
        //std::cout << req_msg->DebugString() << std::endl;
        //std::cout << req_msg->ShortDebugString() << std::endl;

        // 将 Message 转换为 JSON 字符串，方便阅读
        google::protobuf::util::MessageToJsonString(*req_msg, &req_msg_str);
        std::cout << req_msg_str << std::endl;
      } else {
        std::cout << "获取请求参数失败." << std::endl;
      }
    }

    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_MESSAGE)) { //客户端收到返回消息之后
      std::cout << "---InterceptionHookPoints::POST_RECV_MESSAGE---" << std::endl;

      //routeguide::Feature* resp = static_cast<routeguide::Feature*>(methods->GetRecvMessage());
      grpc::protobuf::Message* resp = static_cast<grpc::protobuf::Message*>(methods->GetRecvMessage());
      std::string resp_str;

      if (resp != nullptr) {
        std::cout << "RPC接口: "<< info_->method() << ", ";
        std::cout << "返回参数: " << resp->GetTypeName() << std::endl;
        // 将 Message 转换为 JSON 字符串，方便阅读
        google::protobuf::util::MessageToJsonString(*resp, &resp_str);
        std::cout << resp_str << std::endl;
      } else {
        // 对于流式 RPC 调用结束时，会进入此分支
        std::cout << "RPC接口: "<< info_->method() << ", 接收返回消息结束." << std::endl;
      }
      
    }

    if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::POST_RECV_STATUS)) {
      std::cout << "---InterceptionHookPoints::POST_RECV_STATUS---" << std::endl;

      grpc::Status* status = methods->GetRecvStatus();
      std::string resp_msg = status->ok()?"成功":status->error_message();
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
  grpc::experimental::ClientRpcInfo* info_;
  routeguide::Point req_msg_point;
  routeguide::Rectangle req_msg_rect;
  routeguide::RouteNote req_msg_route;
};

class ClientLoggingInterceptorFactory
    : public grpc::experimental::ClientInterceptorFactoryInterface {
 public:
  grpc::experimental::Interceptor* CreateClientInterceptor(
      grpc::experimental::ClientRpcInfo* info) override {
    return new ClientLoggingInterceptor(info);
  }
};

#endif //_LOG_INTERCEPTOR_CLIENT_H_
