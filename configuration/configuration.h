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
    std::string data_path;
    std::string address;
    std::string username;
    std::string passwd;
    int         port;
    int         scan_dur;
    int         log_dur;
    int         max_hold;
};

struct ftp_file_name {
    std::string tentinh;
    std::string tencoso;
    std::string tentram;
};

struct io_name_bind {
    bool        enable;
    std::string hw_name;
    std::string user_name;
    std::string unit;
    std::string pin_calib;
    std::string pin_error;
    std::string tag_desc;
    double      lim_min;
    double      lim_max;
    double      CO2;
    double      Temp;
    double      Press;
    double      min;
    double      max;
    double      coefficient;
    double      start;
};

struct user_config{    
    ftp_server_info server;
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

    void load_ftp_server_config(ftp_server_info &server, ftp_server_info &def_val);
    void save_ftp_server_config(const ftp_server_info &server);

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
