#ifndef FTP_MANAGER_H
#define FTP_MANAGER_H

#include <client/tcpclientsock.h>
#include <regex.h>

using namespace lib::ipc;

namespace app {
namespace client {




class FtpManager : public TcpClientSock
{




public:
    FtpManager();
    ~FtpManager();

    int start(const std::string &iaddress, int iport,
              const std::string &usrname, const std::string &passwd, const std::string &prefix_path);
    int stop();

    int setup_channel();    
    int remote_cwd(const std::string &path, std::string &recv_msg);
    int remote_ls(std::string &recv_msg);
    int remote_pwd(std::string &recv_msg);
    int remote_mkd(const std::string &path, std::string &recv_msg);
    int remote_put(const std::string &file_name, const std::string &local_path,
                   const std::string &remote_path);

    int remote_ctrl_send_recv(const std::string &cmd, std::string &recv_msg);
    int remote_data_send_recv(const std::string &cmd, std::string &recv_msg);
    int remote_raw_cmd(const std::string &cmd);

    int remote_mkd_recursive(const std::string &path);

private:

    std::string username_;
    std::string passwd_;
    std::string ipaddr_;
    std::string prefix_path_;
    int port_;

    typedef int (FtpManager::*FPTR)();
    std::map<std::string, FPTR> handler;
    std::map<std::string, FPTR>::iterator hit;

};




}
}
#endif // TCPCLIENT_H
