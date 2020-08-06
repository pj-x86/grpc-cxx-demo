/**
 * @file route_guide.h
 * @author pj-x86 (pj81102@163.com)
 * @brief 
 * @version 0.1
 * @date 2020-08-05
 * 
 */

#ifndef _ROUTE_GUIDE_H_
#define _ROUTE_GUIDE_H_

#include <iostream>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>

#include "userlog.h"
#include "helper.h"
#include "log_interceptor_server.h"

#include "route_guide.grpc.pb.h"

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

namespace routeguide
{
    /**
     * @brief RouteGuideImpl 实现 protobuf 中定义的服务以及rpc接口
     * 
     */
    class RouteGuideImpl final : public RouteGuide::Service
    {
    public:
        /**
         * @brief Construct a new Route Guide Impl object
         * 
         * @param db 保存地理位置信息的文件数据库
         */
        explicit RouteGuideImpl(const std::string &db)
        {
            routeguide::ParseDb(db, &feature_list_);
        }

        /**
         * @brief 获取 point 位置的 feature 属性（一元RPC）
         * 
         * @param context gRPC的上下文
         * @param point 表示一个特定的地理位置
         * @param feature 某个地理位置上的特点描述
         * @return Status gRPC调用返回结果
         */
        Status GetFeature(ServerContext *context, const Point *point,
                          Feature *feature) override;

        /**
         * @brief 列出 rectangle 矩形区域内的所有特性集合（服务端流RPC）
         * 
         * @param context gRPC的上下文
         * @param rectangle 表示一个特定的矩形区域
         * @param writer 返回流， Feature 数据集合
         * @return Status gRPC调用返回结果
         */
        Status ListFeatures(ServerContext *context,
                            const routeguide::Rectangle *rectangle,
                            ServerWriter<Feature> *writer) override;
        
        /**
         * @brief 统计给定的地理位置集合 reader 中有多少个 Point ，多少个 Feature，位置之间总距离是多少（客户端流RPC）
         * 
         * @param context gRPC的上下文
         * @param reader 输入流，Point 数据集合
         * @param summary 输出统计结果
         * @return Status gRPC调用返回结果
         */
        Status RecordRoute(ServerContext *context, ServerReader<Point> *reader,
                           RouteSummary *summary) override;

        /**
         * @brief 对输入的位置集合进行检索，如果存在相同的位置，则返回该位置信息
         * 
         * @param context gRPC的上下文
         * @param stream 输入输出流
         * @return Status gRPC调用返回结果
         */
        Status RouteChat(ServerContext *context,
                         ServerReaderWriter<RouteNote, RouteNote> *stream) override;

    private:
        std::vector<Feature> feature_list_;
        std::mutex mu_;
        std::vector<RouteNote> received_notes_;
    };

} // namespace routeguide

#endif //_ROUTE_GUIDE_H_