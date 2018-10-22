#ifndef TCP_SERVER_CLIENT_H
#define TCP_SERVER_CLIENT_H

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>


#include <cpp_lib/ipc.h>
#include <cpp_lib/thread.h>
#include <cpp_lib/debug.h>
#include <jsoncpp/json/json.h>
#include <cpp_lib/datalog.h>

#include <configuration.h>
#include <usermanager.h>

using namespace lib::ipc;

namespace app {
namespace server {

enum {
    CMD_READ_TIME = 0,
    CMD_READ_RAW_DATA,
    CMD_READ_INT_DATA,
    CMD_MAX,
};

struct AvgValue{
    double inter_value_avg;
    double final_value_avg;
    int count;
    double total_inter;
    double total_final;
    double curr_inter;
    double curr_final;
};

class TcpClient
{
    std::shared_ptr<lib::ipc::endpoint> csock_;
    volatile bool       run_;
    volatile bool       run_bg_;
    std::thread         thread_listen_;
    std::thread         thread_bg_;
    lib::ipc::STATUS    status_;
    std::function<void(lib::ipc::STATUS stat)> notify_;
    std::function<void(const void *data, int len)> data_handler_;
    std::shared_ptr<lib::ipc::tcp::client::configuration> config_;    
    void create_thread();
    std::tm *time_now_;
    uint32_t send_num_;
    double time_start_, time_step_;
    std::string     user_name_;
    std::string     role_;
    bool     logged_in_;
    bool     streaming_;

    user_config userconfig_;

    std::vector<AvgValue> list_avg_value_;
    int update_rate_second_;
    int counter_rate_;

public:
    explicit TcpClient(std::shared_ptr<lib::ipc::endpoint> &sock);
    virtual ~TcpClient();
    bool start();
    void stop();
    void net_notify(std::function<void(lib::ipc::STATUS stat)> func) {
        if(func)  notify_ = func;
    }

    void set_data_handler(std::function<void(const void *data, int len)> func) {
        if(func) data_handler_ = func;
    }

    lib::ipc::STATUS get_status() { return status_; }
    void set_status(lib::ipc::STATUS stat) {status_ = stat;}
    int send_data(const void *data, int len);
    void listen_thread();
    void bg_thread();
    void process_raw_data(const void *data, int len);
    void send_realtime_timedate();
    void send_realtime_timedate_avg();
    void send_current_time();


    void send_config_all_tag();
    void send_tag_info(int tag);    
    void send_system_info();
    void set_network(bool dhcp, const std::string &ipaddress, const std::string &netmask);
    void send_status_message(const std::string &type, const std::string &status,
                             const std::string &message);

    static bool get_if_ipaddress(const std::string &if_name,
                                 std::string &ipaddress,
                                 std::string &netmask,
                                 std::string &broadcast);
    void send_list_account();
    static void push_point_avg(AvgValue &point, double inter, double final);
    static void cal_point_avg(AvgValue &point);
    void process_avg_value();
    void get_all_avg_value();
};




}
}
#endif // TCP_SERVER_CLIENT_H
