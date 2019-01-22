#ifndef TAGREAD_H
#define TAGREAD_H

#include <DCTag.h>
#include <string>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <configuration.h>

#define COMPILE_ADAM3600        0


#if COMPILE_ADAM3600 == 1
#define api_tag_open     dc_tag_open
#define api_tag_close    dc_tag_close
#define api_tag_add      dc_tag_add
#define api_tag_remove   dc_tag_remove
#define api_tag_init     dc_tag_init
#define api_tag_uninit   dc_tag_uninit
#define api_tag_read     dc_tag_read
#else
#define api_tag_open
#define api_tag_close
#define api_tag_add
#define api_tag_remove
#define api_tag_init
#define api_tag_uninit
#define api_tag_read
#endif




namespace app {
class tagread
{
public:
    tagread(io_name_bind config);

    ~tagread();    
    void tag_close();

    std::string get_hw_name()   const {return io_bind_.hw_name;}
    std::string get_usr_name()  const {return io_bind_.user_name;}
    std::string get_tag_inter_unit()  const {return io_bind_.inter_unit;}
    std::string get_tag_final_unit()  const {return io_bind_.final_unit;}

    double  get_o2_comp() const {return io_bind_.ai_o2_comp;}
    double  get_temp_comp() const {return io_bind_.ai_temp_comp;}
    double  get_press_comp() const {return io_bind_.ai_press_comp;}

    std::string get_o2_comp_hw_name() const {return io_bind_.ai_o2;}
    std::string   get_temp_comp_hw_name() const {return io_bind_.ai_temp;}
    std::string   get_press_comp_hw_name() const {return io_bind_.ai_press;}

    void    call_inter_value_curr();
    double  get_inter_value() const {return  inter_value_;}
    double  get_raw_readed();

    void    set_final_value(double value);
    double  get_final_value() const {return final_value_;}

    void    set_status(std::string status) {status_ =  status;}
    std::string    get_status() const {return status_;}

    bool    get_raw_value();

    bool    get_tag_enable()    const {return io_bind_.enable;}        

    bool    get_tag_report()    const {return io_bind_.report;}    
    bool    get_tag_report2()    const {return io_bind_.report2;}

    int     get_tag_tram_idx()  const {return io_bind_.tram;}

    std::string get_tag_pin_calib() const {return io_bind_.pin_calib;}
    std::string get_tag_pin_error() const {return io_bind_.pin_error;}
    bool        has_pin_calib() const {return io_bind_.has_calib;}
    bool        has_pin_error() const {return io_bind_.has_error;}

    int     get_final_cal_type() const {return io_bind_.final_type;}
    bool    get_final_cal_revert() const {return io_bind_.cal_revert;}

private:
    io_name_bind    io_bind_;
    DC_TAG_HANDLE   handle_;
    DC_TAG          value_;     // raw value    
    double          final_value_;
    double          inter_value_;
    std::string     status_;

    double          range_raw;
    double          range_cal;
};


class tagmanager : public lib::pattern::singleton<tagmanager>
{
public:
    ~tagmanager();

    void deinit_all_tag() {
        #if COMPILE_ADAM3600 == 1
        api_tag_uninit();
        #endif
    }

    void set_num_tag(int numtag);
    bool add_tag(io_name_bind config);
    bool get_inter_value_by_username(const std::string &name, double &value);
    bool get_inter_value_by_hwname(const std::string &name, double &value);
    bool get_raw_value_by_hwname(const std::string &name, double &value);
    void scan_all_raw_inter_value();
    void calculate_final_value();
    std::vector<std::shared_ptr<tagread>> get_tag_list() {return tag_list_;}
    std::vector<std::shared_ptr<tagread>>  tag_list_;

private:

    int                 num_tag_;
};
}


#endif // TAGREAD_H
