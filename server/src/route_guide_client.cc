/**
 * @file route_guide_client.cc
 * @author pj-x86 (pj81102@163.com)
 * @brief 客户端主程序入口
 * @version 0.1
 * @date 2020-08-05
 * 
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "userlog.h"

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "helper.h"
#include "log_interceptor_client.h"

#include "route_guide.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using routeguide::Point;
using routeguide::Feature;
using routeguide::Rectangle;
using routeguide::RouteSummary;
using routeguide::RouteNote;
using routeguide::RouteGuide;

Point MakePoint(long latitude, long longitude) {
  Point p;
  p.set_latitude(latitude);
  p.set_longitude(longitude);
  return p;
}

Feature MakeFeature(const std::string& name,
                    long latitude, long longitude) {
  Feature f;
  f.set_name(name);
  f.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return f;
}

RouteNote MakeRouteNote(const std::string& message,
                        long latitude, long longitude) {
  RouteNote n;
  n.set_message(message);
  n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return n;
}

class RouteGuideClient {
 public:
  RouteGuideClient(std::shared_ptr<Channel> channel, const std::string& db)
      : stub_(RouteGuide::NewStub(channel)) {
    routeguide::ParseDb(db, &feature_list_);
  }

  void GetFeature() {
    Point point;
    Feature feature;
    point = MakePoint(409146138, -746188906);
    GetOneFeature(point, &feature);
    point = MakePoint(0, 0);
    GetOneFeature(point, &feature);
  }

  void ListFeatures() {
    routeguide::Rectangle rect;
    Feature feature;
    ClientContext context;
    
    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    //std::cout << "Looking for features between 40, -75 and 42, -73"
    //          << std::endl;
    SPDLOG_INFO("Looking for features between 40, -75 and 42, -73");

    std::unique_ptr<ClientReader<Feature> > reader(
        stub_->ListFeatures(&context, rect));
    while (reader->Read(&feature)) {
      // std::cout << "Found feature called "
      //           << feature.name() << " at "
      //           << feature.location().latitude()/kCoordFactor_ << ", "
      //           << feature.location().longitude()/kCoordFactor_ << std::endl;
      SPDLOG_INFO("Found feature called {} at {:f}, {:f}", feature.name(), 
        feature.location().latitude()/kCoordFactor_, feature.location().longitude()/kCoordFactor_);
    }
    Status status = reader->Finish();
    if (status.ok()) {
      //std::cout << "ListFeatures rpc succeeded." << std::endl;
      SPDLOG_INFO("ListFeatures rpc succeeded.");
    } else {
      //std::cout << "ListFeatures rpc failed." << std::endl;
      SPDLOG_ERROR("ListFeatures rpc failed.");
    }
  }

  void RecordRoute() {
    Point point;
    RouteSummary stats;
    ClientContext context;
    const int kPoints = 10;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> feature_distribution(
        0, feature_list_.size() - 1);
    std::uniform_int_distribution<int> delay_distribution(
        500, 1500);

    std::unique_ptr<ClientWriter<Point> > writer(
        stub_->RecordRoute(&context, &stats));
    for (int i = 0; i < kPoints; i++) {
      const Feature& f = feature_list_[feature_distribution(generator)];
      // std::cout << "Visiting point "
      //           << f.location().latitude()/kCoordFactor_ << ", "
      //           << f.location().longitude()/kCoordFactor_ << std::endl;
      SPDLOG_INFO("Visiting point {:f}, {:f}", f.location().latitude()/kCoordFactor_, f.location().longitude()/kCoordFactor_);
      if (!writer->Write(f.location())) {
        // Broken stream.
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(
          delay_distribution(generator)));
    }
    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
      // std::cout << "Finished trip with " << stats.point_count() << " points\n"
      //           << "Passed " << stats.feature_count() << " features\n"
      //           << "Travelled " << stats.distance() << " meters\n"
      //           << "It took " << stats.elapsed_time() << " seconds"
      //           << std::endl;
      SPDLOG_INFO("Finished trip with {:d} points", stats.point_count());
      SPDLOG_INFO("Passed {:d} features", stats.feature_count());
      SPDLOG_INFO("Travelled {:d} meters", stats.distance());
      SPDLOG_INFO("It took {:d} seconds", stats.elapsed_time());
    } else {
      //std::cout << "RecordRoute rpc failed." << std::endl;
      SPDLOG_ERROR("RecordRoute rpc failed.");
    }
  }

  void RouteChat() {
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<RouteNote, RouteNote> > stream(
        stub_->RouteChat(&context));

    std::thread writer([stream]() {
      std::vector<RouteNote> notes{
        MakeRouteNote("First message", 0, 0),
        MakeRouteNote("Second message", 0, 1),
        MakeRouteNote("Third message", 1, 0),
        MakeRouteNote("Fourth message", 0, 0)};
      for (const RouteNote& note : notes) {
        // std::cout << "Sending message " << note.message()
        //           << " at " << note.location().latitude() << ", "
        //           << note.location().longitude() << std::endl;
        SPDLOG_INFO("Sending message {} at {:d}, {:d}", note.message(), note.location().latitude(), note.location().longitude());
        stream->Write(note);
      }
      stream->WritesDone();
    });

    RouteNote server_note;
    while (stream->Read(&server_note)) {
      // std::cout << "Got message " << server_note.message()
      //           << " at " << server_note.location().latitude() << ", "
      //           << server_note.location().longitude() << std::endl;
      SPDLOG_INFO("Got message {} at {:d}, {:d}", server_note.message(), 
        server_note.location().latitude(), server_note.location().longitude());
    }
    writer.join();
    Status status = stream->Finish();
    if (!status.ok()) {
      //std::cout << "RouteChat rpc failed." << std::endl;
      SPDLOG_ERROR("RouteChat rpc failed.");
    }
  }

 private:

  bool GetOneFeature(const Point& point, Feature* feature) {
    ClientContext context;
    Status status = stub_->GetFeature(&context, point, feature);
    if (!status.ok()) {
      //std::cout << "GetFeature rpc failed. error_message=" << status.error_message() << std::endl;
      SPDLOG_ERROR("GetFeature rpc failed. error_message={}", status.error_message());
      
      return false;
    }
    if (!feature->has_location()) {
      //std::cout << "Server returns incomplete feature." << std::endl;
      SPDLOG_WARN("Server returns incomplete feature.");
      return false;
    }
    if (feature->name().empty()) {
      // std::cout << "Found no feature at "
      //           << feature->location().latitude()/kCoordFactor_ << ", "
      //           << feature->location().longitude()/kCoordFactor_ << std::endl;
      SPDLOG_INFO("Found no feature at {:f}, {:f}", 
        feature->location().latitude()/kCoordFactor_, feature->location().longitude()/kCoordFactor_ );
    } else {
      // std::cout << "Found feature called " << feature->name()  << " at "
      //           << feature->location().latitude()/kCoordFactor_ << ", "
      //           << feature->location().longitude()/kCoordFactor_ << std::endl;
      SPDLOG_INFO("Found feature called {} at {:f}, {:f}", feature->name(), 
        feature->location().latitude()/kCoordFactor_, feature->location().longitude()/kCoordFactor_);
    }
    return true;
  }

  const float kCoordFactor_ = 10000000.0;
  std::unique_ptr<RouteGuide::Stub> stub_;
  std::vector<Feature> feature_list_;
};

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

int main(int argc, char** argv) {

  int iRet = 0;

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

    // 初始化日志框架
    init_logger("dev","../logs/client","debug");

    std::string db = routeguide::GetDbFileContent(gConfigInfo.FileDBPath);

    //创建带客户端拦截器的 Channel 实例
    grpc::ChannelArguments args;
    std::vector<
        std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
        interceptor_creators;
    interceptor_creators.push_back(std::unique_ptr<ClientLoggingInterceptorFactory>(
        new ClientLoggingInterceptorFactory()));
    auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
        "localhost:"+gConfigInfo.ServerPort, grpc::InsecureChannelCredentials(), args,
        std::move(interceptor_creators));

    //auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    RouteGuideClient guide(channel, db);

    //std::cout << "-------------- GetFeature --------------" << std::endl;
    SPDLOG_INFO("-------------- GetFeature --------------");
    guide.GetFeature();
    //std::cout << "-------------- ListFeatures --------------" << std::endl;
    SPDLOG_INFO("-------------- ListFeatures --------------");
    guide.ListFeatures();
    //std::cout << "-------------- RecordRoute --------------" << std::endl;
    SPDLOG_INFO("-------------- RecordRoute --------------");
    if (argc < 2) {
      //std::cout << "请先指定参数: --db_path=xxx.json" << std::endl;
      //std::cout << "示例: --db_path=../../route_guide_db.json" << std::endl;
      SPDLOG_ERROR("请先指定参数: --db_path=xxx.json");
      SPDLOG_ERROR("示例: --db_path=../../route_guide_db.json");
    } else {
      guide.RecordRoute();  
    }
    
    //std::cout << "-------------- RouteChat --------------" << std::endl;
    SPDLOG_INFO("-------------- RouteChat --------------");
    guide.RouteChat();

    SPDLOG_INFO("应用退出");

    //退出日志框架
    exit_logger();

    return 0;
}
