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
    std::string raw_value;
    std::string final_value;
    std::string unit;
    std::string status;
};
class RealtimeData : public lib::pattern::singleton<RealtimeData>
{
public:


    RealtimeData();
    ~RealtimeData() {
        destroy();
    }

    void add_point_value(const app::io_name_bind &tag);
    bool set_raw_value(const std::string &hw_name, double value);
    bool set_final_value(const std::string &hw_name, double value);
    bool set_status(const std::string &hw_name, const std::string &status);

    std::vector<RealtimeValue> get_list_value() const {return list_value_;}

private:
    std::vector<RealtimeValue>      list_value_;


};

}
#endif // REALTIMEVALUE_H
