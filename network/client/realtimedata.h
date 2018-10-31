#ifndef REALTIMEDATA_H
#define REALTIMEDATA_H

#include <cpp_lib/pattern.h>
#include <vector>
#include <configuration.h>
namespace app {
struct RealtimeValue{
    bool        enable;
    std::string hw_name;
    std::string sw_name;
    double inter_value;
    double final_value;
    std::string inter_unit;
    std::string final_unit;
    std::string status;
    double      alarm;
    double      min;
    double      max;
    bool        alarm_enable;
};
class RealtimeData : public lib::pattern::singleton<RealtimeData>
{
public:


    RealtimeData();
    ~RealtimeData() {
        destroy();
    }

    void add_point_value(const app::io_name_bind &tag);
    bool set_inter_value(const std::string &hw_name, double value);
    bool set_final_value(const std::string &hw_name, double value);
    bool set_status(const std::string &hw_name, const std::string &status);
    bool set_all_value(const std::string &hw_name, double final,
                       double inter, const std::string &status);

    int get_stream_inter_duration() const {return stream_inter_dur_;}
    int get_stream_final_duration() const {return stream_final_dur_;}
    void set_stream_inter_duration(int duration) {stream_inter_dur_ = duration;}
    void set_stream_final_duration(int duration) {stream_final_dur_ = duration;}

    std::vector<RealtimeValue> get_list_value() const {return list_value_;}

private:
    std::vector<RealtimeValue>      list_value_;
    int stream_inter_dur_;
    int stream_final_dur_;


};

}
#endif // REALTIMEVALUE_H
