#ifndef TCPCLIENT_SOCK_H
#define TCPCLIENT_SOCK_H

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
#include <regex.h>




using namespace lib::ipc;

namespace app {
namespace client {

class TcpClientSock
{
public:
    enum FtpCode {
        FTP_ERR_NONE = 0,
        FTP_ERR_FILE,
        FTP_ERR_PUT,
        FTP_ERR_GET,
        FTP_ERR_CD,
        FTP_ERR_NCD,
        FTP_ERR_MKD,
        FTP_ERR_PWD,
        FTP_ERR_OPEN_CTRL_CONN,
        FTP_ERR_OPEN_DATA_CONN,
        FTP_ERR_RECV_CTRL_MSG,
        FTP_ERR_RECV_DATA_MSG,
        FTP_ERR_SEND_CTRL_MSG,
        FTP_ERR_SEND_DATA_MSG,
        FTP_ERR_AUTHEN,
        FTP_ERR_PASV,
        FTP_ERR_CODE
    };

    struct FileDesc {
        std::string local_file_path;
        std::string remote_file_path;
        std::string file_name;
    };
private:
    std::shared_ptr<lib::ipc::endpoint> ctrlsock_;
    std::shared_ptr<lib::ipc::endpoint> datasock_;

    lib::ipc::STATUS  ctrl_sock_status_;
    lib::ipc::STATUS  data_sock_status_;

    bool is_run_data_;
    bool is_run_control_;

    std::thread thread_data_;
    std::thread thread_control_;



public:
    TcpClientSock();
    ~TcpClientSock();

    int start_sock(std::shared_ptr<lib::ipc::endpoint> &sock,
                   const std::string &ip, int port);

    int stop_sock(std::shared_ptr<lib::ipc::endpoint> &sock);

    int sock_send_data(std::shared_ptr<lib::ipc::endpoint> &sock,
                       const void *data, int len);

    int start_ctrl_sock(const std::string &ip, int port);
    int stop_ctrl_sock();

    int start_data_sock(const std::string &ip, int port);
    int stop_data_sock();

    int send_ctrl_msg(const std::string &msg);
    int recv_ctrl_msg(std::string &msg, int timeout);

    int send_data_msg(const std::string &msg);
    int send_data_msg(const uint8_t *data, int len);
    int recv_data_msg(std::string &msg, int timeout);

    int start_data_channel();

    lib::ipc::STATUS get_ctrl_sock_status() { return ctrl_sock_status_; }
    void set_ctrl_sock_status(lib::ipc::STATUS stat) {ctrl_sock_status_ = stat;}

    lib::ipc::STATUS get_data_sock_status() { return data_sock_status_; }
    void set_data_sock_status(lib::ipc::STATUS stat) {data_sock_status_ = stat;}

    void listen_data();
    void listen_control();

    void stop();

    int get_ip_port(const std::string msg, std::string &ip, int &port);

    int get_code(const std::string &msg);

    std::string glob_ctrl_msg_;
    std::string glob_data_msg_;
};




}
}
#endif // TCPCLIENT_H
