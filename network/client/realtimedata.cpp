#include "realtimedata.h"

namespace app {
RealtimeData::RealtimeData()
{
    list_value_.clear();
}

void RealtimeData::add_point_value(const app::io_name_bind &tag) {
    RealtimeValue point;
    point.enable = tag.enable;
    point.hw_name = tag.hw_name;
    point.sw_name = tag.user_name;
    point.unit = tag.unit;
    list_value_.push_back(point);
}

bool RealtimeData::set_raw_value(const std::string &hw_name, double value)
{
    for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.raw_value = std::to_string(value);
            return true;
        }
    }
    return false;
}

bool RealtimeData::set_final_value(const std::string &hw_name, double value)
{
    for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.final_value = std::to_string(value);
            return true;
        }
    }
    return false;
}

bool RealtimeData::set_status(const std::string &hw_name, const std::string &status)
{
    for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.status = status;
            return true;
        }
    }
    return false;
}

}
