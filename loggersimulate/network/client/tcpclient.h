#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <cpp_lib/ipc.h>
#include <cpp_lib/thread.h>
#include <cpp_lib/debug.h>
#include <cpp_lib/convert.h>

#include <jsoncpp/json/json.h>




using namespace lib::ipc;

namespace app {
namespace client {

class TcpClient
{
    std::shared_ptr<lib::ipc::endpoint> csock_;
    volatile bool   run_;
    std::thread     thread_listener_;
    lib::ipc::STATUS  status_;
    std::function<void(lib::ipc::STATUS stat)> notify_;
    std::function<void(const void *data, int len)> data_handler_;
    std::shared_ptr<lib::ipc::tcp::client::configuration> config_;
    void create_thread();
    struct tm* time_now_;
    std::string ipaddress_;
    int         ipaddress_port_;
public:
    TcpClient();
    ~TcpClient();
    int start(const std::string &address, int port);
    bool stop();
    void set_notify(std::function<void(lib::ipc::STATUS stat)> func) {
        if(func)  notify_ = func;
    }

    void set_recv_event(std::function<void(const void *data, int len)> func) {
        if(func) data_handler_ = func;
    }

    lib::ipc::STATUS get_status() { return status_; }
    void set_status(lib::ipc::STATUS stat) {status_ = stat;}
    int send_data(const void *data, int len);
    int send_data_wlen(const void *data, int len);
    void listen_function();
    void process_raw_data(const void *data, int len);
};




}
}
#endif // TCPCLIENT_H
