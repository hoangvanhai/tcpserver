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
    bool read_value(DC_TAG &value);
    void tag_close();

    std::string get_hw_name()   const {return io_bind_.hw_name;}
    std::string get_usr_name()  const {return io_bind_.user_name;}
    std::string get_tag_unit()  const {return io_bind_.unit;}
    double  get_tag_coeff()      const {return io_bind_.coefficient;}
    double  get_tag_min()       const {return io_bind_.min;}
    double  get_tag_max()       const {return io_bind_.max;}
    bool    get_tag_enable()    const {return io_bind_.enable;}
    void    set_tag_enable(bool en) {io_bind_.enable = en;}
    void    set_tag_min(double min) {io_bind_.min = min;}
    void    set_tag_max(double max) {io_bind_.max = max;}
    double  get_tag_start() const {return io_bind_.start;}
    double  get_tag_avg_value();
    std::string get_tag_pin_calib() const {return io_bind_.pin_calib;}
    std::string get_tag_pin_error() const {return io_bind_.pin_error;}
    double  get_tag_value();
    void    get_tag_value(DC_TAG &value) {value = value_;}
    double get_tag_final_value();

private:
    io_name_bind    io_bind_;
    DC_TAG_HANDLE   handle_;
    DC_TAG          value_;     // raw value
    double          final_value_;
    double          value_avg_;
    int             read_times_;
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
    bool get_user_tag_value(const std::string &name, DC_TAG &value);
    bool get_hw_tag_value(const std::string &name, DC_TAG &value);
    void get_all_tag_value();
    std::vector<std::shared_ptr<tagread>> get_tag_list() {return tag_list_;}
    std::vector<std::shared_ptr<tagread>>  tag_list_;

private:

    int                 num_tag_;
};
}


#endif // TAGREAD_H
