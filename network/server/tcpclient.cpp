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
    streaming_ = false;
    user_name_.clear();
    role_.clear();
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
    //send_config_all_tag();
    while(run_bg_) {
        if(streaming_)
            send_realtime_data();
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
            std::string ipaddress, netmask;
            bool dhcp;

            user_config cfg = app::userconfig::instance()->get_user_config();
            cfg.server.address = smsg["serverip"].asString();
            cfg.server.passwd = smsg["password"].asString();
            cfg.server.port = smsg["port"].asInt();
            cfg.server.username = smsg["username"].asString();
            cfg.server.log_dur = smsg["logdur"].asInt();
            ipaddress = smsg["ipaddress"].asString();
            netmask = smsg["netmask"].asString();
            dhcp = smsg["dhcp"].asBool();

            cfg.filename.tentinh = smsg["tinh"].asString();
            cfg.filename.tencoso = smsg["coso"].asString();
            cfg.filename.tentram = smsg["tram"].asString();

            app::userconfig::instance()->save_user_config(cfg);


            set_network(dhcp, ipaddress, netmask);

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
//                cfg.tag[tag + 12].enable = smsg["enable"].asBool();
//                cfg.tag[tag + 12].user_name = smsg["sw"].asString();
//                cfg.tag[tag + 12].unit = smsg["unit"].asString();
//                cfg.tag[tag + 12].pin_calib = smsg["calib"].asString();
//                cfg.tag[tag + 12].pin_error = smsg["error"].asString();
//                cfg.tag[tag + 12].min = smsg["min"].asDouble();
//                cfg.tag[tag + 12].max = smsg["max"].asDouble();
//                cfg.tag[tag + 12].coefficient = smsg["coeff"].asDouble();
//                cfg.tag[tag + 12].start = smsg["start"].asDouble();
//                cfg.tag[tag + 12].tag_desc = smsg["desc"].asString();
//                cfg.tag[tag + 12].lim_min = smsg["lim_min"].asDouble();
//                cfg.tag[tag + 12].lim_max = smsg["lim_max"].asDouble();

                app::userconfig::instance()->save_user_config(cfg);
                send_status_message("set_tag_info", "success", "set tag successful");
            } else {
                LREP("tag id not number\n");
            }
        } else if(value["subtype"] == "system_reboot") {
            sync();
            setuid(0);
            reboot(RB_AUTOBOOT);
            LREP("reboot system\n");
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
        if(logged_in_ && (role_ == "supperuser" || role_ == "admin")) {
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
        if(logged_in_ && role_ == "supperuser") {
            if(value["data"] != "") {
                smsg = value["data"];
                if(smsg["username"].asString() != "root") {
                    if(UsersLogin::instance()->delUser(smsg["username"].asString())) {
                        send_status_message("del_user", "success", "delete user successful");
                    } else {
                        send_status_message("del_user", "error", "data value");
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
        if(logged_in_ && role_ == "supperuser") {
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


void TcpClient::send_realtime_data()
{    
    Json::Value root, array, time;
    std::vector<app::RealtimeValue> vect = app::RealtimeData::instance()->get_list_value();
    std::time_t t = std::time(0);
    time_now_ = std::localtime(&t);

    root["type"] = "realtime_data";
    for(auto &var : vect) {
        Json::Value value;
//        value["enable"] = var.enable;
//        value["hw"] = var.hw_name;
//        value["sw"] = var.sw_name;
//        value["raw"] = var.raw_value;
//        value["final"] = var.final_value;
//        value["unit"] = var.unit;
//        value["status"] = var.status;
        array.append(value);
    }

    time["year"] = std::to_string(time_now_->tm_year + 1900);
    time["month"] = std::to_string(time_now_->tm_mon + 1);
    time["day"] = std::to_string(time_now_->tm_mday);
    time["hour"] = std::to_string(time_now_->tm_hour);
    time["min"] = std::to_string(time_now_->tm_min);
    time["sec"] = std::to_string(time_now_->tm_sec);

    root["data"] = array;
    root["time"] = time;
    //std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::send_config_all_tag()
{
    Json::Value root, array;
    user_config config = userconfig::instance()->get_user_config();

    root["type"] = "config_file";

    for(int i = 0; i < 2; i++) {
        Json::Value value;
//        value["enable"] = config.tag[i].enable;
//        value["hw"] = config.tag[i].hw_name;
//        value["sw"] = config.tag[i].user_name;
//        value["unit"] = config.tag[i].unit;
//        value["calib"] = config.tag[i].pin_calib;
//        value["error"] = config.tag[i].pin_error;
//        value["min"] = config.tag[i].min;
//        value["max"] = config.tag[i].max;
//        value["coeff"] = config.tag[i].coefficient;
//        value["start"] = config.tag[i].start;
        array.append(value);
    }

    root["data"] = array;
    std::cout << root << std::endl;
    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::send_tag_info(int tag)
{
    Json::Value root, value;
    user_config config = userconfig::instance()->get_user_config();

    root["type"] = "control";
    root["subtype"] = "get_tag_info";
    root["tag_id"] = tag;

//    value["enable"] = config.tag[tag + 12].enable;
//    value["hw"] = config.tag[tag + 12].hw_name;
//    value["sw"] = config.tag[tag + 12].user_name;
//    value["unit"] = config.tag[tag + 12].unit;
//    value["calib"] = config.tag[tag + 12].pin_calib;
//    value["error"] = config.tag[tag + 12].pin_error;
//    value["min"] = config.tag[tag + 12].min;
//    value["max"] = config.tag[tag + 12].max;
//    value["coeff"] = config.tag[tag + 12].coefficient;
//    value["start"] = config.tag[tag + 12].start;
//    value["desc"] = config.tag[tag + 12].tag_desc;
//    value["lim_min"] = config.tag[tag + 12].lim_min;
//    value["lim_max"] = config.tag[tag + 12].lim_max;
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
    get_if_ipaddress("ens33", myip, netmask, broadcast);
    root["type"] = "control";
    root["subtype"] = "get_system_info";
    sysinfo["ipaddress"] = myip;
    sysinfo["netmask"] = netmask;
    sysinfo["tinh"] = config.filename.tentinh;
    sysinfo["coso"] = config.filename.tencoso;
    sysinfo["tram"] = config.filename.tentram;

    sysinfo["serverip"] = config.server.address;
    sysinfo["port"] = config.server.port;
    sysinfo["username"] = config.server.username;
    sysinfo["password"] = config.server.passwd;
    sysinfo["logdur"] = config.server.log_dur;

    root["data"] = sysinfo;

    Json::FastWriter fast_writer;
    std::string msg = fast_writer.write(root);
    send_data(msg.c_str(), (int)msg.size());
}

void TcpClient::set_network(bool dhcp,
                            const std::string &ipaddress,
                            const std::string &netmask)
{
    std::string dhcp_cmd;
    if(dhcp) {
        dhcp_cmd = "sed -i ':a;N;$!ba;s/iface enp7s0 inet static/iface enp7s0 inet dhcp/g' /etc/network/interfaces" ;
        system(dhcp_cmd.c_str());
    } else {
        dhcp_cmd = "sed -i ':a;N;$!ba;s/iface enp7s0 inet dhcp/iface enp7s0 inet static/g' /etc/network/interfaces" ;
        std::string ip_cmd = "sed -i ':a;N;$!ba;s/address [0-9]\{0,3\}\.[0-9]\{0,3\}\.[0-9]\{0,3\}\.[0-9]\{0,3\}/address " +
                ipaddress + "/g' + /etc/network/interfaces";
        std::string netmask_cmd = "sed -i ':a;N;$!ba;s/netmask [0-9]\{0,3\}\.[0-9]\{0,3\}\.[0-9]\{0,3\}\.[0-9]\{0,3\}/netmask " +
                netmask + "/g' + /etc/network/interfaces";

        system(dhcp_cmd.c_str());
        std::this_thread::sleep_for(500_ms);
        system(ip_cmd.c_str());
        std::this_thread::sleep_for(500_ms);
        system(netmask_cmd.c_str());
    }

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


}
} //namespace app

