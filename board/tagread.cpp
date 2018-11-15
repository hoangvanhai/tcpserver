#include "tagread.h"
#include <cpp_lib/debug.h>
#include <cpp_lib/rand.h>
#include <client/realtimedata.h>

namespace app {

tagread::tagread(io_name_bind config)
{    
    final_value_ = 0;
    inter_value_ = 0;
    io_bind_ = config;
#if COMPILE_ADAM3600 == 1
    handle_ = api_tag_open( io_bind_.hw_name.c_str(), NULL );
#endif
    if ( nullptr == handle_ ) {
        std::cout << "handle is null, open tag failed: " <<
                     io_bind_.hw_name << std::endl;
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

void tagread::call_inter_value_curr()
{
    double X = io_bind_.rang_min + (value_.value / range_raw) * range_cal ;
    inter_value_ = io_bind_.coef_a + io_bind_.coef_b * X;
}

void tagread::set_final_value(double value)
{
    final_value_ = value;
}

bool tagread::get_raw_value()
{
    if(handle_ != nullptr) {        
        int err = api_tag_read(&handle_, &value_, 1);
        //std::cout << "tag: " << io_bind_.user_name << " val: " << value_.value << std::endl;
        if(!err) {
            return true;
        }                
    }
    value_.value = lib::rand::generate(500000.0, 1000000.0); //500000;//
    return false;
}

double tagread::get_raw_readed()
{
    return value_.value;
}


tagmanager::~tagmanager()
{
    tag_list_.clear();
    deinit_all_tag();
}

void tagmanager::set_num_tag(int numtag) {
    num_tag_ = numtag;
    int err;
    err = api_tag_init(num_tag_, 0, 0);
    if(err) {
        std::cout << "dc tag init error " << err << std::endl;
    } else {
        std::cout << "dc tag init success\n";
    }
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

bool tagmanager::get_inter_value_by_username(
        const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_usr_name() == name) {
            value = var->get_inter_value();
            return true;
        }
    }
    return false;
}


bool tagmanager::get_inter_value_by_hwname(
        const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_hw_name() == name) {
            value = var->get_inter_value();
            return true;
        }
    }
    return false;
}


bool tagmanager::get_raw_value_by_hwname(
        const std::string &name, double &value)
{
    for(auto &var : tag_list_) {
        if(var->get_hw_name() == name) {
            value = var->get_raw_readed();
            return true;
        }
    }
    return false;
}

void tagmanager::scan_all_raw_inter_value()
{
    for(auto &var : tag_list_) {
        var->get_raw_value();
        var->call_inter_value_curr();
    }
}

void tagmanager::calculate_final_value()
{
    scan_all_raw_inter_value();
    for(auto &var : tag_list_) {
    if(var->get_hw_name().find("BoardIO:AI") != std::string::npos) {
        double value_calib, value_error;
        double value;
        std::string status = "";

        if(var->has_pin_calib() || var->has_pin_error()) {
            status = "00";
        }

        if(var->has_pin_calib())
        {
            if(!app::tagmanager::instance()->get_raw_value_by_hwname(
                        var->get_tag_pin_calib(),
                        value)) {
                return;
            }
            value_calib = value;           
            if(value_calib > 0) {
                status = "01";
            }
        }

        if(var->has_pin_error()) {
            if(!app::tagmanager::instance()->get_raw_value_by_hwname(
                        var->get_tag_pin_error(),
                        value)) {
                return;
            }

            value_error = value;
            if(value_error > 0) {
                status = "02";
            }
        }



        double final_value, inter_value;
        bool cal_revert = var->get_final_cal_revert();
        switch(var->get_final_cal_type()) {
        case 0:
            inter_value = var->get_inter_value();
            final_value = inter_value;
            break;
        case 1: {
                inter_value = var->get_inter_value();
                double o2_comp = var->get_o2_comp();
                double get_value;
                double o2_coeff = 1;
                if(get_inter_value_by_hwname(
                            var->get_o2_comp_hw_name(),
                            get_value)) {
                    double den = 20.9 - get_value;
                    if(den != 0 && (20.9 - o2_comp) != 0) {
                        o2_coeff = ((20.9 - o2_comp) / den);
                    }
                } else {
                    //WARN("hw name not found {}\r\n", var->get_o2_comp_hw_name());
                }

                if(!cal_revert)
                    final_value = inter_value * o2_coeff;
                else
                    final_value = inter_value / o2_coeff;

                //WARN("{} O2 COMP: inter value = {}, final value = {} o2_comp: {} \r\n", var->get_usr_name(), inter_value, final_value, o2_comp);
            }
            break;
        case 2: {
            inter_value = var->get_inter_value();
            double press_comp = var->get_press_comp();
            double temp_comp = var->get_temp_comp();
            double localvalue, temp_coeff = 1, press_coeff = 1;
            if(get_inter_value_by_hwname(
                        var->get_press_comp_hw_name(),
                        localvalue)) {
                if(localvalue != 0 && press_comp != 0)
                    press_coeff = press_comp / localvalue;
            }

            if(get_inter_value_by_hwname(
                        var->get_temp_comp_hw_name(),
                        localvalue)) {

//                if(((273 + localvalue) != 0) && ((273 + temp_comp) != 0))
//                    temp_coeff = (273 + temp_comp) / (273 + localvalue);

                if(((273 + localvalue) != 0) && ((273 + temp_comp) != 0))
                    temp_coeff = (273 + localvalue) / (273 + temp_comp);
            }

            if(!cal_revert)
                final_value = inter_value * temp_coeff * press_coeff;
            else
                final_value = inter_value / (temp_coeff * press_coeff);

            //WARN("{} T-P COMP: inter value = {}, final value = {} t-comp: {} p-comp: {} \r\n", var->get_usr_name(),
                 //inter_value, final_value, temp_comp, press_comp);
        }
            break;
        default:
            break;
        }

        if(final_value < 0) final_value = 0;
        if(status == "01" || status == "02") {
            final_value = 0;
            inter_value = 0;
        }

        var->set_final_value(final_value);
        var->set_status(status);

        app::RealtimeData::instance()->set_all_value(
                var->get_hw_name(),
                final_value, inter_value, status);
        }
    }
}



}
