#include "tcpclient.h"
#include <ctime>
#include <cpp_lib/convert.h>
#include <cpp_lib/rand.h>
#include <client/realtimedata.h>

#include <unistd.h>
#include <sys/reboot.h>


namespace app {
namespace server {

TcpClient::TcpClient(std::shared_ptr<endpoint> &sock) :
    csock_(sock), run_(false), run_bg_(false)
{    
    status_ = lib::ipc::STATUS_CONNECTED;
    send_num_ = 0;
    logged_in_ = false;
    streaming_ = true;
    user_name_.clear();
    role_.clear();
    AvgValue point;
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    list_avg_value_.push_back(point);
    update_rate_second_ = 0;
    userconfig_ = userconfig::instance()->get_user_config();
}

TcpClient::~TcpClient()
{
    stop();
}

bool TcpClient::start()
{    
    if(!csock_) return false;
    create_thread();    
    return true;
}

void TcpClient::stop()
{
    run_ = false;
    if(thread_listen_.joinable())
        thread_listen_.join();
    if(csock_)
        csock_->close_connection();

    run_bg_ = false;
    if(thread_bg_.joinable())
        thread_bg_.join();
}

int TcpClient::send_data(const void *data, int len)
{
    int err = -1;
    if(data == NULL || len <= 0) {
        return err;
    }

    int event = csock_->wait_event(100, false, true);

    if(event & EVENT_WRITEABLE) {
        std::shared_ptr<packet> pkg = std::make_shared<packet>(data, len);
        err = (int)csock_->write_data(pkg);
        pkg.reset();
    }

    return err;
}

void TcpClient::create_thread()
{
    run_ = true;
    thread_listen_ = std::thread([&]() {
        listen_thread();
    });

    run_bg_ = true;
    thread_bg_ = std::thread([&]() {
       bg_thread();
    });
}


void TcpClient::listen_thread()
{
    int err_count = 0;
    int quit_code = 0;
    while (run_) {        
        int event = csock_->wait_event(100, true, false);
        if (event & EVENT_ERROR) {
            err_count++;
            if(err_count > 10) {
                LREP("select failed\n");
                quit_code = -1;
                break;
            }
        } else if (event & EVENT_READABLE) {
            std::ssize_t rlen = csock_->read_data([&](const std::shared_ptr<output> &ipkg) {
                std::shared_ptr<packet> pkg = std::dynamic_pointer_cast<packet>(ipkg);
                if (!pkg) {
                    return lib::ipc::READ_ACTION_BREAK;
                } else {
                    if(pkg->data_size() > 0) {
                        process_raw_data(pkg->data(), (int)pkg->data_size());
                    }
                }
                return lib::ipc::READ_ACTION_CONTINUE;
            });

            if (rlen <= 0) {
                LREP("read failed\n");
                quit_code  = -2;
                break;
            }
        }        
    }
    LREP("close connection run: {} code: {}\n", run_, quit_code);
    csock_->close_connection();
    status_ = lib::ipc::STATUS_DISCONNECTED;
}

void TcpClient::bg_thread()
{
    while(run_bg_) {

        if(streaming_) {
            if(update_rate_second_ == 0) {
                send_realtime_timedate();
            } else {
                process_avg_value();
                counter_rate_++;
                if(counter_rate_ >= update_rate_second_) {
                    get_all_avg_value();
                    send_realtime_timedate_avg();
                    counter_rate_ = 0;
                } else {
                    send_current_time();
                }
            }
        }
        std::this_thread::sleep_for(1_s);
    }
}
/**
 * @brief TcpClient::process_raw_data
 * @param data
 * @param len
 *
 * LREP("read raw data leng {}\n", len);
    send_data(data, len);

    TestMessage msg;
    if(msg.ParseFromArray(data, len)) {
        //LREP(msg.DebugString());
        LREP("clientid: {} clientname: {} description: {}\n",
             msg.clientid(), msg.clientname(), msg.description());

        for(auto item : msg.messageitems()) {
            LREP("id: {} name: {} value: {} type: {}\n",
                 item.id(), item.itemname(), item.itemvalue(), item.itemtype());
        }

    } else {
        LREP("parse failed\n");
    }


 */
void TcpClient::process_raw_data(const void *data, int len)
{
    char *sdata = (char*)data;
    Json::Reader reader;    
    Json::Value value;    
    Json::Value smsg;
    reader.parse((const char*)data, (const char*)(sdata + len - 1), value);
    std::cout << "recv msg: " << value << std::endl;
    if(value["type"] == "control") {
        if(value["subtype"] == "streaming") {
            if(value["data"] != "") {
                smsg = value["data"];
                if(smsg["streaming"].asString() == "on") {
                    streaming_ = true;
                } else if(smsg["streaming"].asString() == "off") {
                    streaming_ = false;
                    WARN("stop streaming {}...\n", 1);
                }
            } else {
                std::cout << smsg << std::endl;
            }
        } else if(value["subtype"] == "get_system_info") {
            send_system_info();
        } else if(value["subtype"] == "set_system_info") {
            smsg = value["data"];
            //std::string ipaddress, netmask;

            user_config cfg = app::userconfig::instance()->get_user_config();
            cfg.server.enable  = smsg["enable"].asBool();
            cfg.server.address = smsg["serverip"].asString();
            cfg.server.prefix_path = smsg["prefix"].asString();
            cfg.server.passwd = smsg["password"].asString();
            cfg.server.username = smsg["username"].asString();
            cfg.server.log_dur = smsg["logdur"].asDouble();

            cfg.server2.enable  = smsg["enable2"].asBool();
            cfg.server2.address = smsg["serverip2"].asString();
            cfg.server2.prefix_path = smsg["prefix2"].asString();
            cfg.server2.passwd = smsg["password2"].asString();
            cfg.server2.username = smsg["username2"].asString();
            cfg.server2.log_dur = smsg["logdur2"].asDouble();




            cfg.filename.tentinh = smsg["tinh"].asString();
            cfg.filename.tencoso = smsg["coso"].asString();
            cfg.filename.tentram = smsg["tram"].asString();

//            set_network(false, ipaddress, netmask);

            app::userconfig::instance()->save_user_config(cfg);           
            send_status_message("set_system_info", "success", "save tag param success");

        } else if(value["subtype"] == "get_tag_info") {
            if(value["tag_id"].isInt()) {
                int tag = value["tag_id"].asInt();
                LREP("send tag info {}\n", tag);
                send_tag_info(tag);
            }

        } else if(value["subtype"] == "set_tag_info") {
            LREP("set tag info\n");
            if(value["tag_id"].isInt()) {
                int tag = value["tag_id"].asInt();
                if(tag >= 8 || tag < 0) {
                    ERR("tag number \n");
                    return;
                }
                smsg = value["data"];
                user_config cfg = app::userconfig::instance()->get_user_config();
                cfg.tag[tag + 12].report = smsg["report"].asBool();
                cfg.tag[tag + 12].report2 = smsg["report2"].asBool();
                cfg.tag[tag + 12].cal_revert = smsg["cal_revert"].asBool();
                cfg.tag[tag + 12].alarm_enable = smsg["alarm_en"].asBool();
                cfg.tag[tag + 12].user_name = smsg["sw"].asString();
                cfg.tag[tag + 12].final_unit = smsg["final_unit"].asString();
                cfg.tag[tag + 12].inter_unit = smsg["inter_unit"].asString();
                cfg.tag[tag + 12].pin_calib = "BoardIO:DI." + smsg["calib"].asString();
                cfg.tag[tag + 12].has_calib = smsg["has_calib"].asBool();
                cfg.tag[tag + 12].pin_error = "BoardIO:DI." + smsg["error"].asString();
                cfg.tag[tag + 12].has_error = smsg["has_error"].asBool();
                cfg.tag[tag + 12].rang_min = smsg["min"].asDouble();
                cfg.tag[tag + 12].rang_max = smsg["max"].asDouble();
                cfg.tag[tag + 12].coef_a = smsg["coef_a"].asDouble();
                cfg.tag[tag + 12].coef_b = smsg["coef_b"].asDouble();
                cfg.tag[tag + 12].final_type = StdCondType(smsg["final_type"].asInt());
                cfg.tag[tag + 12].tag_desc = smsg["desc"].asString();
                cfg.tag[tag + 12].ai_o2_comp = smsg["o2_comp"].asDouble();
                cfg.tag[tag + 12].ai_temp_comp = smsg["temp_comp"].asDouble();
                cfg.tag[tag + 12].ai_press_comp = smsg["press_comp"].asDouble();

                cfg.tag[tag + 12].ai_press_unit = smsg["press_unit"].asString();
                cfg.tag[tag + 12].ai_o2_unit = smsg["o2_unit"].asString();
                cfg.tag[tag + 12].ai_temp_unit = smsg["temp_unit"].asString();
                cfg.tag[tag + 12].alarm = smsg["alarm"].asDouble();

                cfg.tag[tag + 12].ai_o2 = "BoardIO:AI." + smsg["ai_o2"].asString();
                cfg.tag[tag + 12].ai_temp = "BoardIO:AI." + smsg["ai_temp"].asString();
                cfg.tag[tag + 12].ai_press = "BoardIO:AI." + smsg["ai_press"].asString();


                app::userconfig::instance()->save_user_config(cfg);
                send_status_message("set_tag_info", "success", "set tag successful");
            } else {
                LREP("tag id not number\n");
            }
        } else if(value["subtype"] == "system_reboot") {
            exit(0);
            sync();
            setuid(0);
            reboot(RB_AUTOBOOT);
            LREP("reboot system\n");
        } else if(value["subtype"] == "get_list_account") {
            send_list_account();
        } else if(value["subtype"] == "update_rate") {
            smsg = value["data"];
            if(smsg["rate"] != "") {
                if(smsg["rate"].asDouble() > 0) {
                    update_rate_second_ = userconfig_.server.log_dur * 60;
                } else {
                    update_rate_second_ = 0;
                }
                WARN("rate = {}\n", update_rate_second_);
            }
        }

    } else if(value["type"] == "config_file") {
        if(value["data"] != "") {
            smsg = value["data"];
        } else {
            std::cout << smsg << std::endl;
        }
    } else if(value["type"] == "login") {
        if(value["data"] != "") {
            smsg = value["data"];
            if(UsersLogin::instance()->checkUserPasswdMatch(smsg["username"].asString(),
                                                            smsg["password"].asString(),
                                                            role_)) {
                user_name_ = smsg["username"].asString();
                logged_in_ = true;
                //send_status_message("login", "success", "login suceess: " + user_name_ + " role: " + role_);
                {
                    Json::Value root;
                    root["type"] = "login";
                    root["status"] = "success";
                    root["username"] = user_name_;
                    root["role"] = role_;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    send_data(msg.c_str(), (int)msg.size());
                    LREP("login success send info\n");
                }
            } else {
                send_status_message("login", "error", "data value");
            }
        } else {
            send_status_message("login", "error", "data format");
        }
    }else if(value["type"] == "logout") {
        logged_in_ = false;
        role_.clear();
        user_name_.clear();
        send_status_message("logout", "success", "user logged out");
    } else if(value["type"] == "add_user") {
        if(logged_in_ && (role_ == "service" || role_ == "admin")) {
            if(value["data"] != "") {
                smsg = value["data"];
                if(UsersLogin::instance()->addUser(smsg["username"].asString(),
                                                   smsg["password"].asString(),
                                                   smsg["role"].asString())) {
                    send_status_message("add_user", "success", "add user successful");
                } else {
                    send_status_message("add_user", "error", "data value");
                }
            } else {
                send_status_message("add_user", "error", "data format");
            }
        } else {
            send_status_message("add_user", "error", "no privilege or not logged in");
        }
    } else if(value["type"] == "del_user") {
        if(logged_in_ && role_ != "user") {
            if(value["data"] != "") {
                smsg = value["data"];
                if(smsg["username"].asString() != "Vatcom") {
                    if(UsersLogin::instance()->delUser(smsg["username"].asString())) {
                        send_status_message("del_user", "success", "delete user successful");
                    } else {
                        send_status_message("del_user", "error", "user not existed");
                    }
                } else {
                    send_status_message("del_user", "error", "can not delete root");
                }
            } else {
                send_status_message("del_user", "error", "data format");
            }
        } else {
            send_status_message("del_user", "error", "no privilege or not logged in");
        }
    } else if(value["type"] == "change_password") {
        if(logged_in_) {
            if(value["data"] != "") {
                smsg = value["data"];
                std::string temp;
                if(UsersLogin::instance()->checkUserPasswdMatch(user_name_, smsg["oldpassword"].asString(), temp)) {
                    if(UsersLogin::instance()->changePasswd(user_name_, smsg["newpassword"].asString())) {
                        send_status_message("change_password", "success", "change password successful");
                    } else {
                        send_status_message("change_password", "error", "data value");
                    }
                } else {
                    send_status_message("change_password", "error", "old password not valid");
                }
            } else {
                send_status_message("change_password", "error", "data format");
            }
        }
    } else if(value["type"] == "reset_password") {
        if(logged_in_ && role_ != "user") {
            if(value["data"] != "") {
                smsg = value["data"];
                if(UsersLogin::instance()->resetPasswd(smsg["username"].asString())) {
                    send_status_message("reset_password", "success", "reset password successful");
                } else {
                    send_status_message("reset_password", "error", "data value");
                }
            } else {
                send_status_message("reset_password", "error", "data format");
            }
        } else {
            send_status_message("reset_password", "error", "no privilege or not logged in");
        }
    } else {
        std::cout << value << std::endl;
    }


    if(data_handler_)
        data_handler_(data, len);
}


void TcpClient::send_realtime_timedate()
{    
    Json::Value root, array, time;
    std::vector<app::RealtimeValue> vect = app::RealtimeData::instance()->get_list_value();
    std::time_t t = std::time(0);
    time_now_ = std::localtime(&t);

    std::string year, month, mday, hour, min, sec;
    year.resize(4);month.resize(2);mday.resize(2);hour.resize(2);min.resize(2);sec.resize(2);

    root["type"] = "realtime_data";
    for(auto &var : vect) {
        Json::Value value;
        value["enable"] = var.enable;
        value["sw"] = var.sw_name;
        value["inter"] = var.inter_value;
        value["final"] = var.final_value;
        value["inter_unit"] = var.inter_unit;
        value["final_unit"] = var.final_unit;
        value["alarm"] = var.alarm;
        value["alarm_en"] = var.alarm_enable;
        value["status"] = var.status;
        value["min"] = var.min;
        value["max"] = var.max;
        array.append(value);
    }

    sprintf((char*)year.data(), "%04d", time_now_->tm_year + 1900);
    sprintf((char*)month.data(), "%02d", time_now_->tm_mon + 1);
    sprintf((char*)mday.data(), "%02d", time_now_->tm_mday);
    sprintf((char*)hour.data(), "%02d", time_now_->tm_hour);
    sprintf((char*)min.data(), "%02d", time_now_->tm_min);
    sprintf((char*)sec.data(), "%02d", time_now_->tm_sec);

    time["year"] = year;
    time["month"] = month;
    time["day"] = mday;
    time["hour"] = hour;
    time["min"] = min;
    time["sec"] = sec;

    root["data"] = array;
    root["time"] = time;
    //std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::send_realtime_timedate_avg()
{
    Json::Value root, array, time;
    std::vector<app::RealtimeValue> vect = app::RealtimeData::instance()->get_list_value();
    std::time_t t = std::time(0);
    time_now_ = std::localtime(&t);

    std::string year, month, mday, hour, min, sec;
    year.resize(4);month.resize(2);mday.resize(2);hour.resize(2);min.resize(2);sec.resize(2);

    root["type"] = "realtime_data";
    for(int i = 0; i < vect.size(); i++) {
        Json::Value value;
        value["enable"] = vect.at(i).enable;
        value["sw"] = vect.at(i).sw_name;
        value["inter"] = list_avg_value_.at(i).inter_value_avg;;
        value["final"] = list_avg_value_.at(i).final_value_avg;
        value["inter_unit"] = vect.at(i).inter_unit;
        value["final_unit"] = vect.at(i).final_unit;
        value["alarm"] = vect.at(i).alarm;
        value["alarm_en"] = vect.at(i).alarm_enable;
        value["status"] = vect.at(i).status;
        value["min"] = vect.at(i).min;
        value["max"] = vect.at(i).max;
        array.append(value);
    }

    sprintf((char*)year.data(), "%04d", time_now_->tm_year + 1900);
    sprintf((char*)month.data(), "%02d", time_now_->tm_mon + 1);
    sprintf((char*)mday.data(), "%02d", time_now_->tm_mday);
    sprintf((char*)hour.data(), "%02d", time_now_->tm_hour);
    sprintf((char*)min.data(), "%02d", time_now_->tm_min);
    sprintf((char*)sec.data(), "%02d", time_now_->tm_sec);

    time["year"] = year;
    time["month"] = month;
    time["day"] = mday;
    time["hour"] = hour;
    time["min"] = min;
    time["sec"] = sec;

    root["data"] = array;
    root["time"] = time;
    //std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::send_current_time()
{
    Json::Value root, array, time;
    std::time_t t = std::time(0);
    time_now_ = std::localtime(&t);

    std::string year, month, mday, hour, min, sec;
    year.resize(4);month.resize(2);mday.resize(2);hour.resize(2);min.resize(2);sec.resize(2);
    root["type"] = "realtime_data";
    sprintf((char*)year.data(), "%04d", time_now_->tm_year + 1900);
    sprintf((char*)month.data(), "%02d", time_now_->tm_mon + 1);
    sprintf((char*)mday.data(), "%02d", time_now_->tm_mday);
    sprintf((char*)hour.data(), "%02d", time_now_->tm_hour);
    sprintf((char*)min.data(), "%02d", time_now_->tm_min);
    sprintf((char*)sec.data(), "%02d", time_now_->tm_sec);

    time["year"] = year;
    time["month"] = month;
    time["day"] = mday;
    time["hour"] = hour;
    time["min"] = min;
    time["sec"] = sec;

    root["data"] = array;
    root["time"] = time;
    //std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::process_avg_value()
{
    std::vector<app::RealtimeValue> vect =
            app::RealtimeData::instance()->get_list_value();

    if(list_avg_value_.size() != vect.size()) {
        ERR("list size does not match\n");
    }

    for(int i = 0; i < vect.size(); i++) {
        push_point_avg(list_avg_value_.at(i),
                       vect.at(i).inter_value,
                       vect.at(i).final_value);
    }

}

void TcpClient::get_all_avg_value()
{
    for(int i = 0; i < list_avg_value_.size(); i++) {
        cal_point_avg(list_avg_value_.at(i));
    }
}



void TcpClient::send_tag_info(int tag)
{
    Json::Value root, value;
    user_config config = userconfig::instance()->get_user_config();
    std::string last;

    root["type"] = "control";
    root["subtype"] = "get_tag_info";
    root["tag_id"] = tag;

    value["report"] = config.tag[tag + 12].report;
    value["report2"] = config.tag[tag + 12].report2;
    value["sw"] = config.tag[tag + 12].user_name;
    value["inter_unit"] = config.tag[tag + 12].inter_unit;
    value["final_unit"] = config.tag[tag + 12].final_unit;
    value["min"] = config.tag[tag + 12].rang_min;
    value["max"] = config.tag[tag + 12].rang_max;
    value["coef_a"] = config.tag[tag + 12].coef_a;
    value["coef_b"] = config.tag[tag + 12].coef_b;
    value["final_type"] = config.tag[tag + 12].final_type;
    value["desc"] = config.tag[tag + 12].tag_desc;


    value["o2_comp"] = config.tag[tag + 12].ai_o2_comp;
    value["temp_comp"] = config.tag[tag + 12].ai_temp_comp;
    value["press_comp"] = config.tag[tag + 12].ai_press_comp;

    value["o2_unit"] = config.tag[tag + 12].ai_o2_unit;
    value["temp_unit"] = config.tag[tag + 12].ai_temp_unit;
    value["press_unit"] = config.tag[tag + 12].ai_press_unit;
    value["alarm"] = config.tag[tag + 12].alarm;
    value["cal_revert"] = config.tag[tag + 12].cal_revert;
    value["alarm_en"] = config.tag[tag + 12].alarm_enable;

    last = std::string((char*)(&config.tag[tag + 12].pin_calib.back()));
    value["calib"] = last;
    value["has_calib"] = config.tag[tag + 12].has_calib;
    last = std::string((char*)(&config.tag[tag + 12].pin_error.back()));

    value["error"] = last;
    value["has_error"] = config.tag[tag + 12].has_error;

    last = std::string((char*)(&config.tag[tag + 12].ai_o2.back()));
    value["ai_o2"] = last;
    last = std::string((char*)(&config.tag[tag + 12].ai_temp.back()));
    value["ai_temp"] = last;
    last = std::string((char*)(&config.tag[tag + 12].ai_press.back()));
    value["ai_press"] = last;

    root["data"] = value;

    std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::send_system_info()
{
    Json::Value root, sysinfo;
    user_config config = userconfig::instance()->get_user_config();
    std::string myip, netmask, broadcast;
    get_if_ipaddress("eth0", myip, netmask, broadcast);
    root["type"] = "control";
    root["subtype"] = "get_system_info";
    sysinfo["ipaddress"] = myip;
    sysinfo["netmask"] = netmask;
    sysinfo["tinh"] = config.filename.tentinh;
    sysinfo["coso"] = config.filename.tencoso;
    sysinfo["tram"] = config.filename.tentram;

    sysinfo["enable"] = config.server.enable;
    sysinfo["prefix"] = config.server.prefix_path;
    sysinfo["serverip"] = config.server.address;
    sysinfo["port"] = config.server.port;
    sysinfo["username"] = config.server.username;
    sysinfo["password"] = config.server.passwd;
    sysinfo["logdur"] = config.server.log_dur;

    sysinfo["enable2"] = config.server2.enable;
    sysinfo["prefix2"] = config.server2.prefix_path;
    sysinfo["serverip2"] = config.server2.address;
    sysinfo["port2"] = config.server2.port;
    sysinfo["username2"] = config.server2.username;
    sysinfo["password2"] = config.server2.passwd;
    sysinfo["logdur2"] = config.server2.log_dur;

    root["data"] = sysinfo;

    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

int TcpClient::set_network(bool dhcp,
                            const std::string &ipaddress,
                            const std::string &netmask)
{

    std::string ip_cmd = "sed -i ':a;N;$!ba;s/address [0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}/address " +
            ipaddress + "/g' /etc/network/interfaces.d/eth0";
    std::string netmask_cmd = "sed -i ':a;N;$!ba;s/netmask [0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}\\.[0-9]\\{0,3\\}/netmask " +
            netmask + "/g' /etc/network/interfaces.d/eth0";

    LREP("set if = {}\n", ip_cmd);

    int ret = system(ip_cmd.c_str());
    if(ret != 0) {
        WARN("ret change ip = {}\r\n", ret);
    }
    ret = system(netmask_cmd.c_str());
    if(ret != 0) {
        WARN("ret change netmask = {}\r\n", ret);
    }

    return ret;
}

void TcpClient::send_status_message(const std::string &type,
                                    const std::string &status,
                                    const std::string &message)
{
    Json::Value root;
    root["type"] = type;
    root["status"] = status;
    root["msg"] = message;
    //std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

bool TcpClient::get_if_ipaddress(const std::string &if_name,
                                 std::string &ipaddress,
                                 std::string &netmask,
                                 std::string &broadcast)
{
    ipaddress="Unable to get IP Address";
    netmask = "Unable to get Net Mask";
    broadcast = "Unable to get Broadcast";
    bool found = false;
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while(temp_addr != NULL) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if(std::string(temp_addr->ifa_name) == if_name){
                    ipaddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                    netmask = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_netmask)->sin_addr);
                    broadcast = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_ifu.ifu_broadaddr)->sin_addr);
                    found = true;
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
    return found;
}

void TcpClient::send_list_account()
{
    std::vector<std::string> vect;
    UsersLogin::instance()->getListAccount(vect);

    Json::Value root, array;

    root["type"] = "control";
    root["subtype"] = "get_list_account";
    for(auto &var : vect) {
        Json::Value value;
        value["account"] = var;
        array.append(value);
    }
    root["data"] = array;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());

}

void TcpClient::push_point_avg(AvgValue &point, double inter, double final)
{
    point.count++;
    point.total_final+= final;
    point.total_inter+= inter;
    point.curr_final = final;
    point.curr_inter = inter;
}

void TcpClient::cal_point_avg(AvgValue &point)
{
    if(point.count > 0) {
        point.final_value_avg = point.total_final /  point.count;
        point.inter_value_avg = point.total_inter /  point.count;
    } else {
        point.inter_value_avg = point.curr_inter;
        point.final_value_avg = point.curr_final;
    }
    point.count = 0;
    point.total_final = 0;
    point.total_inter = 0;
}


}
} //namespace app

