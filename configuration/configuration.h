/**
  @file configuration.h
  @author Haihv14
  */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <cpp_lib/configuration.h>
#include <cpp_lib/pattern.h>
#include <cpp_lib/convert.h>
#include <jsoncpp/json/json.h>
#include <iostream>

#define APP_CONFIG_FILE "config.json"

#define APP_SYSTEM_MAX_TAG      20

namespace app {

struct ftp_server_info {
    bool        enable;
    std::string prefix_path;
    std::string data_path;
    std::string address;
    std::string username;
    std::string passwd;
    int         port;
    int         scan_dur;
    double      log_dur;
    int         max_hold;
};

struct ftp_file_name {
    int         ntram;
    std::string tentinh;
    std::string tencoso;
    std::string tentram;
    std::string tentram2;
};

enum StdCondType {
    Type_None = 0,
    Type_O2 = 1,
    Type_TempPress,
};

struct io_name_bind {
    bool        enable;
    bool        report;
    bool        report2;
    bool        cal_revert;
    bool        alarm_enable;
    int         tram;
    StdCondType final_type;

    std::string tag_desc;
    std::string hw_name;
    std::string user_name;
    std::string inter_unit;
    std::string final_unit;
    bool        has_calib;
    std::string pin_calib;
    bool        has_error;
    std::string pin_error;
    std::string ai_o2;
    std::string ai_o2_unit;
    std::string ai_temp;
    std::string ai_temp_unit;
    std::string ai_press;
    std::string ai_press_unit;


    double      ai_o2_comp;
    double      ai_temp_comp;
    double      ai_press_comp;

    double      rang_min;
    double      rang_max;
    double      raw_min;
    double      raw_max;
    double      coef_a;
    double      coef_b;
    double      alarm;
//    double      raw_value;
//    double      inter_value;
//    double      final_value;
};

struct user_config{    
    ftp_server_info server;
    ftp_server_info server2;
    ftp_file_name   filename;
    io_name_bind tag[APP_SYSTEM_MAX_TAG];
};

class userconfig : public lib::pattern::singleton<userconfig>
{
public:
    userconfig();
    ~userconfig();
    bool start(const std::string dir);
    bool open_file(const std::string &filename);
    void close_file();
    bool load_config();
    void write_file();


    void load_tag_config(const std::string &type, io_name_bind &tag, io_name_bind &def_val);
    void save_tag_config(const std::string &type, io_name_bind tag);

    void load_ftp_server_config(ftp_server_info &server, ftp_server_info &def_val, int idx = 0);
    void save_ftp_server_config(const ftp_server_info &server, int idx = 0);

    void load_ftp_filename_config(ftp_file_name &filename, ftp_file_name &def_val);
    void save_ftp_filename_config(const ftp_file_name &filename);

    template<typename T>
    bool try_load_value(Json::Value &node, const std::string &name, T &value, T &defval) {
        try {
            value = lib::convert::string_to<T>(node[name].asString(), defval);
        } catch (std::string err) {
            std::cout << "load value error " << err << std::endl;
            return false;
        }
        return true;
    }

    bool try_get_object(Json::Value &node,
                        const std::string &name,
                        Json::Value &value);




    user_config         get_user_config() {return config_;}
    void                set_user_config(user_config cfg) {config_ = cfg;}
    void                save_user_config(const user_config &cfg);

protected:
    std::string file_name_;
    std::string directory_;
    Json::Value root_;
    bool is_save_;
    user_config config_;
};

}

#endif // CONFIGURATION_H
