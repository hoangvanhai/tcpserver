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

    double  get_inter_value();
    bool    get_raw_value();
    bool    get_tag_enable()    const {return io_bind_.enable;}        
    bool    get_tag_report()    const {return io_bind_.report;}


    void    cal_inter_value_avg();
    double  get_inter_value_avg();


    std::string get_tag_pin_calib() const {return io_bind_.pin_calib;}
    std::string get_tag_pin_error() const {return io_bind_.pin_error;}

    int     get_final_cal_type() const {return io_bind_.final_type;}

private:
    io_name_bind    io_bind_;
    DC_TAG_HANDLE   handle_;
    DC_TAG          value_;     // raw value
    double          inter_value_avg_;
    double          inter_value_total_;
    int             read_times_;
    double          range_raw;
    double          range_cal;
};


class tagmanager {
public:
    tagmanager(int numtag = 1);
    ~tagmanager();

    void deinit_all_tag() {
        #if COMPILE_ADAM3600 == 1
        api_tag_uninit();
        #endif
    }


    bool add_tag(io_name_bind config);
    bool get_inter_value_by_username(const std::string &name, double &value);
    bool get_inter_value_avg_by_username(const std::string &name, double &value);
    bool get_inter_value_avg_by_hwname(const std::string &name, double &value);
    bool get_raw_value_by_hwname(const std::string &name, double &value);
    void scan_all_raw_inter_avg_value();
    std::vector<std::shared_ptr<tagread>> get_tag_list() {return tag_list_;}
    std::vector<std::shared_ptr<tagread>>  tag_list_;

private:

    int                 num_tag_;
};
}


#endif // TAGREAD_H
