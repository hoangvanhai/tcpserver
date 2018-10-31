#include "configuration.h"
#include <fstream>
#include <iostream>
#include <cpp_lib/debug.h>
#include <cpp_lib/convert.h>
#include <sstream>

using namespace lib::configuration;

namespace app {
userconfig::userconfig() :
    is_save_(false)
{

}

userconfig::~userconfig()
{
    close_file();
    destroy();
}

bool userconfig::start(const std::string dir) {
    directory_ = dir;
    if(!open_file(directory_ + APP_CONFIG_FILE)) return false;
    return load_config();
}

bool userconfig::open_file(const std::string &filename)
{
    file_name_ = filename;
    std::ifstream infile(file_name_.c_str());

    if(infile.good()) {
        std::stringstream stream;
        stream << infile.rdbuf();
        Json::Reader reader;

        bool ready =  reader.parse(stream.str(), root_);
        if(ready){

        } else {
            WARN("json parse content from {} failed\n", file_name_);
            return false;
        }
    } else {
        WARN("file: {} not found\n", file_name_);
        is_save_ = true;
    }

    infile.close();

    return true;
}

void userconfig::close_file()
{
    if(is_save_) {
        Json::StyledStreamWriter writer;
        std::ofstream outfile;
        outfile.open(file_name_, std::ios::out);
        if(outfile.is_open()) {
            writer.write(outfile,root_);
            outfile.close();
            LREP("file saved to: {}\n", file_name_);
            is_save_ = false;
        } else {
            LREP("can't open file {}\n", file_name_);
        }
    }
}

/**
 * @brief userconfig::load config from config.json file
 * if this file is existed, read content, otherwise,
 * create file, create default value
 * @return
 */
bool userconfig::load_config()
{

    {
        ftp_server_info def = {"/opt/data/", "127.0.0.1", "FTP-User", "123456a@", 21,  1000, 300, 100000};
        load_ftp_server_config(config_.server, def);
        LREP("ftp_server: address: {}, port: {}, username: {}, passwd: {}, scan: {}, log: {} max_hold: {}\n",
             config_.server.address, config_.server.port, config_.server.username, config_.server.passwd,
             config_.server.scan_dur, config_.server.log_dur, config_.server.max_hold);
    }
    {
        ftp_file_name def = {"AG", "SGCE", "KHI"};
        load_ftp_filename_config(config_.filename, def);
        LREP("ftp_filename: tentinh: {}, tencoso: {}, tentram: {}\n",
             config_.filename.tentinh, config_.filename.tencoso,
             config_.filename.tentram);
    }

    for(int i = 0; i < APP_SYSTEM_MAX_TAG; i++) {
        io_name_bind def;
        def.enable = true;
        def.report = true;
        def.alarm_enable = true;
        def.cal_revert = false;
        if(i <= 7) {
            def.user_name       = "user_tag" + std::to_string(i);
            def.hw_name         = "BoardIO:DI." + std::to_string(i);
            def.final_type      = StdCondType(0);
            def.inter_unit      = "";
            def.final_unit      = "";
            def.pin_calib       = "";
            def.pin_error       = "";
            def.tag_desc        = "";
            def.ai_o2           = "";
            def.ai_press        = "";
            def.ai_temp         = "";
            def.rang_max        = 0;
            def.rang_min        = 0;
            def.raw_max         = 0;
            def.raw_min         = 0;
            def.ai_o2_comp      = 0;
            def.ai_press_comp   = 0;
            def.ai_o2_comp      = 0;
            def.coef_a          = 0;
            def.coef_b          = 0;
            def.alarm           = 0;
            load_tag_config("1.TAG_DI_" + std::to_string(i), config_.tag[i], def);
        } else if(i > 7 && i <= 11) {
            def.user_name       = "user_tag" + std::to_string(i);
            def.hw_name         = "BoardIO:DO." + std::to_string(i - 8);
            def.final_type      = StdCondType(0);
            def.inter_unit      = "";
            def.final_unit      = "";
            def.pin_calib       = "";
            def.pin_error       = "";
            def.tag_desc        = "";
            def.ai_o2           = "";
            def.ai_press        = "";
            def.ai_temp         = "";
            def.rang_max        = 0;
            def.rang_min        = 0;
            def.raw_max         = 0;
            def.raw_min         = 0;
            def.ai_o2_comp      = 0;
            def.ai_press_comp   = 0;
            def.ai_o2_comp      = 0;
            def.coef_a          = 0;
            def.coef_b          = 0;
            def.alarm           = 0;
            load_tag_config("2.TAG_DO_" + std::to_string(i - 8), config_.tag[i], def);
        } else {            
            def.user_name       = "user_tag" + std::to_string(i);
            def.hw_name         = "BoardIO:AI." + std::to_string(i - 12);
            def.final_type      = StdCondType(0);
            def.inter_unit      = "inter_unit";
            def.final_unit      = "final_unit";
            def.pin_calib       = "BoardIO:DI.0";
            def.pin_error       = "BoardIO:DI.1";
            def.tag_desc        = "Mô tả thông số";
            def.ai_o2           = "";
            def.ai_press        = "";
            def.ai_temp         = "";
            def.ai_o2_unit      = "o2_unit";
            def.ai_temp_unit    = "temp_unit";
            def.ai_press_unit   = "press_unit";
            def.rang_max        = 100;
            def.rang_min        = 0;
            def.raw_max         = 1000000;
            def.raw_min         = 0;
            def.ai_o2_comp      = 7;
            def.ai_press_comp   = 760;
            def.ai_temp_comp    = 25;
            def.coef_a          = 0;
            def.coef_b          = 1;
            def.alarm           = 90;
            load_tag_config("3.TAG_AI_" + std::to_string(i - 12), config_.tag[i], def);
        }

//        LREP("tag{}: hw_name: {}, user_name: {}, unit: {}, coefficient: {}\n", i,
//            config_.tag[i].hw_name, config_.tag[i].user_name,
//             config_.tag[i].unit, config_.tag[i].coefficient);

    }

//    std::cout << root_["3.TAG_AI_0"] << std::endl;

    return true;
}

void userconfig::write_file()
{
    if(is_save_) {
        Json::StyledStreamWriter writer;
        std::fstream file;
        file.open(file_name_, std::ios::out);
        if(file.is_open()) {
            writer.write(file,root_);
            //std::cout << root_;
            file.close();
            LREP("file saved to: {}\n", file_name_);
            is_save_ = false;
        } else {
            LREP("can't open file {}\n", file_name_);
            perror("error ");
        }
    }


}


void userconfig::load_tag_config(const std::string &type, io_name_bind &tag, io_name_bind &def_val)
{
    Json::Value value;
    if(try_get_object(root_, type, value)) {
        tag.enable          = value["enable"].asBool();
        tag.report          = value["report"].asBool();
        tag.cal_revert      = value["cal_revert"].asBool();
        tag.alarm_enable    = value["alarm_en"].asBool();
        tag.final_type      = StdCondType(value["final_type"].asInt());
        tag.tag_desc        = value["tag_desc"].asString();
        tag.hw_name         = value["hw_name"].asString();
        tag.user_name       = value["user_name"].asString();
        tag.inter_unit      = value["inter_unit"].asString();
        tag.final_unit      = value["final_unit"].asString();
        tag.has_calib       = value["has_calib"].asBool();
        tag.pin_calib       = value["pin_calib"].asString();
        tag.has_error       = value["has_error"].asBool();
        tag.pin_error       = value["pin_error"].asString();
        tag.ai_o2           = value["ai_o2"].asString();
        tag.ai_temp         = value["ai_temp"].asString();        
        tag.ai_press        = value["ai_press"].asString();
        tag.ai_o2_unit           = value["ai_o2_unit"].asString();
        tag.ai_temp_unit         = value["ai_temp_unit"].asString();
        tag.ai_press_unit        = value["ai_press_unit"].asString();
        tag.ai_o2_comp      = value["ai_o2_comp"].asDouble();
        tag.ai_temp_comp    = value["ai_temp_comp"].asDouble();
        tag.ai_press_comp   = value["ai_press_comp"].asDouble();
        tag.rang_min        = value["rang_min"].asDouble();
        tag.rang_max        = value["rang_max"].asDouble();
        tag.raw_min         = value["raw_min"].asDouble();
        tag.raw_max         = value["raw_max"].asDouble();
        tag.coef_a          = value["coef_a"].asDouble();
        tag.coef_b          = value["coef_b"].asDouble();
        tag.alarm           = value["alarm"].asDouble();

    } else {
        value["enable"]     = def_val.enable;
        value["report"]     = def_val.report;
        value["cal_revert"] = def_val.cal_revert;
        value["alarm_en"]   = def_val.alarm_enable;
        value["final_type"] = def_val.final_type;
        value["tag_desc"]   = def_val.tag_desc;
        value["hw_name"]    = def_val.hw_name;
        value["user_name"]  = def_val.user_name;
        value["inter_unit"] = def_val.inter_unit;
        value["final_unit"] = def_val.final_unit;
        value["pin_calib"]  = def_val.pin_calib;
        value["has_calib"]  = def_val.has_calib;
        value["pin_error"]  = def_val.pin_error;
        value["has_error"]  = def_val.has_error;
        value["ai_o2"]      = def_val.ai_o2;
        value["ai_temp"]    = def_val.ai_temp;
        value["ai_press"]   = def_val.ai_press;
        value["ai_o2_unit"]      = def_val.ai_o2_unit;
        value["ai_temp_unit"]    = def_val.ai_temp_unit;
        value["ai_press_unit"]   = def_val.ai_press_unit;
        value["ai_o2_comp"] = def_val.ai_o2_comp;
        value["ai_temp_comp"] = def_val.ai_temp_comp;
        value["ai_press_comp"] = def_val.ai_press_comp;
        value["rang_min"]   = def_val.rang_min;
        value["rang_max"]   = def_val.rang_max;
        value["raw_min"]    = def_val.raw_min;
        value["raw_max"]    = def_val.raw_max;
        value["coef_a"]     = def_val.coef_a;
        value["coef_b"]     = def_val.coef_b;
        value["alarm"]     = def_val.alarm;


        root_[type]               = value;
        tag                       = def_val;
        is_save_ = true;
    }
}

void userconfig::save_tag_config(const std::string &type, io_name_bind tag)
{
    Json::Value value;
    try_get_object(root_, type, value);
    value["enable"]     = tag.enable;
    value["report"]     = tag.report;
    value["cal_revert"] = tag.cal_revert;
    value["alarm_en"]   = tag.alarm_enable;
    value["final_type"] = tag.final_type;
    value["tag_desc"]   = tag.tag_desc;
    value["hw_name"]    = tag.hw_name;
    value["user_name"]  = tag.user_name;
    value["inter_unit"] = tag.inter_unit;
    value["final_unit"] = tag.final_unit;
    value["pin_calib"]  = tag.pin_calib;
    value["has_calib"]  = tag.has_calib;
    value["pin_error"]  = tag.pin_error;
    value["has_error"]  = tag.has_error;
    value["ai_o2"]      = tag.ai_o2;
    value["ai_temp"]    = tag.ai_temp;
    value["ai_press"]   = tag.ai_press;
    value["ai_o2_unit"]      = tag.ai_o2_unit;
    value["ai_temp_unit"]    = tag.ai_temp_unit;
    value["ai_press_unit"]   = tag.ai_press_unit;
    value["ai_o2_comp"] = tag.ai_o2_comp;
    value["ai_temp_comp"] = tag.ai_temp_comp;
    value["ai_press_comp"] = tag.ai_press_comp;
    value["rang_min"]   = tag.rang_min;
    value["rang_max"]   = tag.rang_max;
    value["raw_min"]    = tag.raw_min;
    value["raw_max"]    = tag.raw_max;
    value["coef_a"]     = tag.coef_a;
    value["coef_b"]     = tag.coef_b;
    value["alarm"]      = tag.alarm;
    root_[type]               = value;
    is_save_ = true;
}

void userconfig::load_ftp_server_config(ftp_server_info &server, ftp_server_info &def_val)
{
    Json::Value value;
    if(try_get_object(root_, "ftp_server", value)) {
        server.data_path = value["data_path"].asString();
        server.address  = value["address"].asString();        
        server.port     = value["port"].asInt();
        server.username = value["username"].asString();
        server.passwd = value["passwd"].asString();
        server.scan_dur = value["scan_dur"].asInt();
        server.log_dur  = value["log_dur"].asInt();
        server.max_hold = value["max_hold"].asInt();
    } else {
        value["data_path"] = def_val.data_path;
        value["address"]    = def_val.address;
        value["port"]       = def_val.port;
        value["username"]   = def_val.username;
        value["passwd"]     = def_val.passwd;
        value["scan_dur"]   = def_val.scan_dur;
        value["log_dur"]    = def_val.log_dur;
        value["max_hold"]   = def_val.max_hold;
        server              = def_val;
        root_["ftp_server"] = value;
        is_save_ = true;
    }
}

void userconfig::save_ftp_server_config(const ftp_server_info &server)
{
    Json::Value value;
    try_get_object(root_, "ftp_server", value);
    value["data_path"]  = server.data_path;
    value["address"]    = server.address;
    value["port"]       = server.port;
    value["username"]   = server.username;
    value["passwd"]     = server.passwd;
    value["scan_dur"]   = server.scan_dur;
    value["log_dur"]    = server.log_dur;
    value["max_hold"]   = server.max_hold;
    root_["ftp_server"] = value;
    is_save_ = true;
}

void userconfig::load_ftp_filename_config(ftp_file_name &filename,
                                          ftp_file_name &def_val)
{
    Json::Value value;
    if(try_get_object(root_, "file_name", value)) {
        filename.tentinh  = value["ten_tinh"].asString();
        filename.tencoso  = value["ten_coso"].asString();
        filename.tentram  = value["ten_tram"].asString();
    } else {
        value["ten_tinh"]  = def_val.tentinh;
        value["ten_coso"]  = def_val.tencoso;
        value["ten_tram"]  = def_val.tentram;
        filename           = def_val;
        root_["file_name"] = value;
        is_save_ = true;
    }
}

void userconfig::save_ftp_filename_config(const ftp_file_name &filename)
{
    Json::Value value;
    try_get_object(root_, "file_name", value);
    value["ten_tinh"]    = filename.tentinh;
    value["ten_coso"]    = filename.tencoso;
    value["ten_tram"]    = filename.tentram;
    root_["file_name"]   = value;
    std::cout << value;
    is_save_ = true;
}

bool userconfig::try_get_object(Json::Value &node,
                                const std::string &name,
                                Json::Value &value)
{
    try {
        value = node[name];
        if(value.isObject()) {
            return true;
        }
    } catch (std::string err) {
        std::cout << "error: " << err << std::endl;
    }
    LREP("{} not found: use create default value\n", name);
    return false;
}

void userconfig::save_user_config(const user_config &cfg)
{
    set_user_config(cfg);
    save_ftp_filename_config(cfg.filename);
    save_ftp_server_config(cfg.server);
    for(int i = 0; i < APP_SYSTEM_MAX_TAG; i++) {
        if(i <= 7) {
            save_tag_config("1.TAG_DI_" + std::to_string(i), cfg.tag[i]);
        } else if(i > 7 && i <= 11) {
            save_tag_config("2.TAG_DO_" + std::to_string(i - 8), cfg.tag[i]);
        } else {
            save_tag_config("3.TAG_AI_" + std::to_string(i - 12), cfg.tag[i]);
        }
    }
    write_file();
}


}
