#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <list>

#include <cpp_lib/ipc.h>
#include <cpp_lib/thread.h>
#include <cpp_lib/debug.h>
#include <cpp_lib/convert.h>
#include <cpp_lib/rand.h>
#include <sys/stat.h>
#include <tagread.h>
#include <server/tcpclient.h>

#define DC_MAX_TAG_COUNT        8

namespace app {
namespace server {

class TcpServer
{
    std::function<void(const void *data, int len)> data_handler_;
    std::shared_ptr<lib::ipc::tcp::server::configuration> config;
    std::list<std::shared_ptr<TcpClient>>       listClient;
    volatile bool run_background;
    volatile bool run_listener;
    std::thread  thread_listener;
    std::thread  thread_background;
    void listen_thread_func();
    void background_thread_func();
    void create_thread();
    void send_all_client(const void *data, int len);
    void create_newconnection(std::shared_ptr<endpoint> sock);
    std::shared_ptr<lib::ipc::endpoint> ssock;
    std::tm *time_now_;
    std::shared_ptr<lib::datalog::CSVFile> loader_;
    int writeen_row_;
    std::string folder_path_;
    std::string writeen_file_name_full_;

public:
    bool start(const std::string &address, int port);
    void stop();
    TcpServer();
    ~TcpServer();

};



}
}
#endif // TCPSERVER_H
