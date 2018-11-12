#include "realtimedata.h"
#include <cpp_lib/debug.h>

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
    point.inter_unit = tag.inter_unit;
    point.final_unit = tag.final_unit;
    point.alarm = tag.alarm;
    point.alarm_enable = tag.alarm_enable;
    point.min = tag.rang_min;
    point.max = tag.rang_max;
    point.final_value = 0;
    point.inter_value = 0;
    list_value_.push_back(point);
}

bool RealtimeData::set_inter_value(const std::string &hw_name, double value)
{
    for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.inter_value = value;
            return true;
        }
    }
    return false;
}

bool RealtimeData::set_final_value(const std::string &hw_name, double value)
{
    for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.final_value = value;
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

bool RealtimeData::set_all_value(const std::string &hw_name, double final, double inter, const std::string &status)
{    
    std::cout << ">";
     for(auto &var : list_value_) {
        if(var.hw_name == hw_name) {
            var.inter_value = inter;
            var.final_value = final;
            var.status = status;
            return true;
        }
    }
    return false;
}

}
