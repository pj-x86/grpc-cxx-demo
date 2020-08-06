#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>

#include "route_guide.h"

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

namespace routeguide
{

    float ConvertToRadians(float num)
    {
        return num * 3.1415926 / 180;
    }

    // The formula is based on http://mathforum.org/library/drmath/view/51879.html
    float GetDistance(const Point &start, const Point &end)
    {
        const float kCoordFactor = 10000000.0;
        float lat_1 = start.latitude() / kCoordFactor;
        float lat_2 = end.latitude() / kCoordFactor;
        float lon_1 = start.longitude() / kCoordFactor;
        float lon_2 = end.longitude() / kCoordFactor;
        float lat_rad_1 = ConvertToRadians(lat_1);
        float lat_rad_2 = ConvertToRadians(lat_2);
        float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
        float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

        float a = pow(sin(delta_lat_rad / 2), 2) + cos(lat_rad_1) * cos(lat_rad_2) *
                                                       pow(sin(delta_lon_rad / 2), 2);
        float c = 2 * atan2(sqrt(a), sqrt(1 - a));
        int R = 6371000; // metres

        return R * c;
    }

    std::string GetFeatureName(const Point &point,
                               const std::vector<Feature> &feature_list)
    {
        for (const Feature &f : feature_list)
        {
            if (f.location().latitude() == point.latitude() &&
                f.location().longitude() == point.longitude())
            {
                //std::cout << "found. name=" << f.name() << std::endl;
                SPDLOG_INFO("found. name={}", f.name());
                return f.name();
            }
        }
        return "";
    }

    Status RouteGuideImpl::GetFeature(ServerContext *context, const Point *point,
                      Feature *feature)
    {
        //std::cout << "latitude=" << point->latitude() << ",longitude=" << point->longitude() << std::endl;
        SPDLOG_INFO("latitude={:d},longitude={:d}", point->latitude(), point->longitude());
        feature->set_name(GetFeatureName(*point, feature_list_));
        feature->mutable_location()->CopyFrom(*point);
        return Status::OK;
        //return grpc::Status(grpc::StatusCode::NOT_FOUND, "test-not-found");
    }

    Status RouteGuideImpl::ListFeatures(ServerContext *context,
                        const routeguide::Rectangle *rectangle,
                        ServerWriter<Feature> *writer)
    {
        auto lo = rectangle->lo();
        auto hi = rectangle->hi();
        long left = (std::min)(lo.longitude(), hi.longitude());
        long right = (std::max)(lo.longitude(), hi.longitude());
        long top = (std::max)(lo.latitude(), hi.latitude());
        long bottom = (std::min)(lo.latitude(), hi.latitude());
        for (const Feature &f : feature_list_)
        {
            if (f.location().longitude() >= left &&
                f.location().longitude() <= right &&
                f.location().latitude() >= bottom &&
                f.location().latitude() <= top)
            {
                writer->Write(f);
            }
        }
        return Status::OK;
    }

    Status RouteGuideImpl::RecordRoute(ServerContext *context, ServerReader<Point> *reader,
                       RouteSummary *summary)
    {
        Point point;
        int point_count = 0;
        int feature_count = 0;
        float distance = 0.0;
        Point previous;

        system_clock::time_point start_time = system_clock::now();
        while (reader->Read(&point))
        {
            point_count++;
            if (!GetFeatureName(point, feature_list_).empty())
            {
                feature_count++;
            }
            if (point_count != 1)
            {
                distance += GetDistance(previous, point);
            }
            previous = point;
        }
        system_clock::time_point end_time = system_clock::now();
        summary->set_point_count(point_count);
        summary->set_feature_count(feature_count);
        summary->set_distance(static_cast<long>(distance));
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time);
        summary->set_elapsed_time(secs.count());

        return Status::OK;
    }

    Status RouteGuideImpl::RouteChat(ServerContext *context,
                     ServerReaderWriter<RouteNote, RouteNote> *stream)
    {
        RouteNote note;
        while (stream->Read(&note))
        {
            std::unique_lock<std::mutex> lock(mu_);
            for (const RouteNote &n : received_notes_)
            {
                if (n.location().latitude() == note.location().latitude() &&
                    n.location().longitude() == note.location().longitude())
                {
                    stream->Write(n);
                }
            }
            received_notes_.push_back(note);
        }

        return Status::OK;
    }

} // namespace routeguide