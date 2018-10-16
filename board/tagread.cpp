#include "tagread.h"
#include <cpp_lib/debug.h>
#include <cpp_lib/rand.h>

namespace app {

tagread::tagread(io_name_bind config)
{
    read_times_ = 0;
    io_bind_ = config;
#if COMPILE_ADAM3600 == 1
    handle_ = api_tag_open( io_bind_.hw_name.c_str(), NULL );
#endif
    if ( nullptr == handle_ ) {
        std::cout << "handle is null, open tag failed: " << io_bind_.hw_name << std::endl;
        api_tag_close(handle_);
    }
}

tagread::~tagread()
{
    tag_close();
}

bool tagread::read_value(DC_TAG &value)
{
    if(handle_ != nullptr) {
        value_.value = lib::rand::generate(100.0, 200.0);
        int err = api_tag_read(&handle_, &value_, 1);
        if(err) {            
            //std::cout << "read error: " << err << std::endl;            
        }
        value = value_;        
        return true;
    }
    value_.value = lib::rand::generate(100.0, 200.0);
    return false;
}

void tagread::tag_close()
{
    api_tag_close(handle_);
}

double tagread::get_tag_avg_value()
{
    read_times_ = 0;
    return value_avg_;
}

double tagread::get_tag_value() {

    double curr_val = value_.value;

    final_value_ =  get_tag_start() +
                    ((value_.value - get_tag_min()) * get_tag_coeff());

    //LREP("value: {} ", value_.value);
    if(read_times_ == 0) {
        value_avg_ = curr_val;
    } else {
        value_avg_ = (value_avg_ + curr_val) / 2;
    }
    read_times_++;
    return curr_val;
}

double tagread::get_tag_final_value()
{
    final_value_ =  get_tag_start() +
                    ((value_.value - get_tag_min()) * get_tag_coeff());

    return final_value_;
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

bool tagmanager::get_user_tag_value(const std::string &name,
                               DC_TAG &value)
{

    for(auto &var : tag_list_) {
        if(var->get_usr_name() == name) {
            var->get_tag_value(value);
            return true;
        }
    }
    //LREP("\r\n");
    return false;
}

bool tagmanager::get_hw_tag_value(const std::string &name, DC_TAG &value)
{
    for(auto &var : tag_list_) {
        if(var->get_hw_name() == name) {
            var->get_tag_value(value);
            return true;
        }
    }
    return false;
}

void tagmanager::get_all_tag_value()
{        
    for(auto &var : tag_list_) {
        DC_TAG value;
        var->read_value(value);        
    }
}


}
