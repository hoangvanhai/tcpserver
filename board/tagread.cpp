#include "tagread.h"
#include <cpp_lib/debug.h>
#include <cpp_lib/rand.h>

namespace app {

tagread::tagread(io_name_bind config)
{
    read_times_ = 0;
    inter_value_total_ = 0;
    io_bind_ = config;
#if COMPILE_ADAM3600 == 1
    handle_ = api_tag_open( io_bind_.hw_name.c_str(), NULL );
#endif
    if ( nullptr == handle_ ) {
        std::cout << "handle is null, open tag failed: " << io_bind_.hw_name << std::endl;
        api_tag_close(handle_);
    }

    range_cal = io_bind_.rang_max - io_bind_.rang_min;
    if(range_cal == 0) range_cal = 1;

    range_raw = io_bind_.raw_max -  io_bind_.raw_min;
    if(range_raw == 0) range_raw = 1;
}

tagread::~tagread()
{
    tag_close();
}


void tagread::tag_close()
{
    api_tag_close(handle_);
}

double tagread::get_inter_value()
{
    double X = io_bind_.rang_min + (value_.value / range_raw) * range_cal ;
    io_bind_.inter_value = io_bind_.coef_a + io_bind_.coef_b * X;
    return io_bind_.inter_value;
}

bool tagread::get_raw_value()
{
    if(handle_ != nullptr) {        
        int err = api_tag_read(&handle_, &value_, 1);
        if(!err) {
            return true;
        }                
    }
    value_.value = lib::rand::generate(100.0, 200.0);
    return false;
}

void tagread::cal_inter_value_avg()
{
    double inter_value = get_inter_value();
    inter_value_total_ += inter_value;
    read_times_++;
}


double tagread::get_inter_value_avg()
{
    double avg = 0;
    if(read_times_ > 0) {
        avg = inter_value_total_ / read_times_;
    } else {
        avg = io_bind_.inter_value;
    }
    read_times_ = 0;
    inter_value_total_ = 0;
    return avg;
}


tagmanager::tagmanager(int numtag)
{
    num_tag_ = numtag;
    int err;
    err = api_tag_init(num_tag_, 0, 0);
    if(err) {
        std::cout << "dc tag init error " << err << std::endl;
    } else {
        std::cout << "dc tag init success\n";
    }

}

tagmanager::~tagmanager()
{
    tag_list_.clear();
    deinit_all_tag();
}

bool tagmanager::add_tag(io_name_bind config)
{
    if(tag_list_.size() >= num_tag_) {
        std::cout << "no slot for add tag\n";
        return false;
    }
    std::shared_ptr<tagread> tag = std::make_shared<tagread>(config);
    tag_list_.push_back(tag);

//    std::cout << "added tag name: " << tag->get_hw_name() << " "
//              << tag->get_usr_name() << " " << tag->get_tag_unit() << " "
//              << tag->get_tag_coeff() << std::endl;

    return true;
}

bool tagmanager::get_inter_value_by_username(const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_usr_name() == name) {
            value = var->get_inter_value();
            return true;
        }
    }
    return false;
}

bool tagmanager::get_inter_value_avg_by_username(const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_usr_name() == name) {
            value = var->get_inter_value_avg();
            return true;
        }
    }
    return false;
}

bool tagmanager::get_inter_value_avg_by_hwname(const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_hw_name() == name) {
            value = var->get_inter_value_avg();
            return true;
        }
    }
    return false;
}

bool tagmanager::get_raw_value_by_hwname(const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_hw_name() == name) {
            value = var->get_raw_value();
            return true;
        }
    }
    return false;
}

void tagmanager::scan_all_raw_inter_avg_value()
{
    for(auto &var : tag_list_) {
        var->get_raw_value();
        var->cal_inter_value_avg();
    }
}


}
