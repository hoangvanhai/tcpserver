#include <client/tcpclientsock.h>
#include <sys/socket.h>

namespace app {
namespace client {

TcpClientSock::TcpClientSock()
{
    ctrlsock_ = std::make_shared<lib::ipc::tcp::client>();
    datasock_ = std::make_shared<lib::ipc::tcp::client>();
    ctrl_sock_status_ = STATUS_DISCONNECTED;
    is_run_control_ = false;
    is_run_data_ = false;    
}


TcpClientSock::~TcpClientSock()
{
    stop();
}

int TcpClientSock::start_sock(std::shared_ptr<endpoint> &sock, const std::string &ip, int port)
{
    int err = -1;
    std::shared_ptr<lib::ipc::tcp::client::configuration> config =
            std::make_shared<lib::ipc::tcp::client::configuration>(ip, port);

    if(config) {
        err = sock->open_connection(config);
    }

    return err;
}

int TcpClientSock::stop_sock(std::shared_ptr<endpoint> &sock)
{
    return sock->close_connection();
}

int TcpClientSock::sock_send_data(std::shared_ptr<endpoint> &sock, const void *data, int len)
{
    int err = -1;
    if(data == NULL || len <= 0) {
        return err;
    }

    int event = sock->wait_event(100, false, true);

    if(event & EVENT_WRITEABLE) {
        err = send(sock->fd(), (const char *)data, len, 0);
    }

    return err;
}


int TcpClientSock::start_ctrl_sock(const std::string &ip, int port)
{
    stop_ctrl_sock();
    int err = start_sock(ctrlsock_, ip, port);
    WARN("try connect to {}:{} err = {}\n", ip, port, err);
    if(err == 0) {
        set_ctrl_sock_status(STATUS_CONNECTED);
        is_run_control_ = true;
        thread_control_ = std::thread([&](){
           listen_control();
        });
    }

    return err;
}

int TcpClientSock::stop_ctrl_sock()
{
    is_run_control_ = false;
    if(thread_control_.joinable())
        thread_control_.join();
    stop_sock(ctrlsock_);
    set_ctrl_sock_status(STATUS_DISCONNECTED);

    return 0;
}

int TcpClientSock::start_data_sock(const std::string &ip, int port)
{
    stop_data_sock();
    int err = start_sock(datasock_, ip, port);
    if(err == 0) {
        set_data_sock_status(STATUS_CONNECTED);
//        is_run_data_ = true;
//        thread_data_ = std::thread([&](){
//           listen_data();
//        });
    }

    return err;
}

int TcpClientSock::stop_data_sock()
{
//    is_run_data_ = false;
//    if(thread_data_.joinable())
//        thread_data_.join();

    stop_sock(datasock_);
    set_data_sock_status(STATUS_DISCONNECTED);
    return 0;
}



int TcpClientSock::send_ctrl_msg(const std::string &msg)
{
    glob_ctrl_msg_.clear();
    glob_data_msg_.clear();
    if(get_ctrl_sock_status() == STATUS_CONNECTED) {

        std::string send_msg = msg + "\r\n";
        //LREP("send msg: {}\n", send_msg);
        return sock_send_data(ctrlsock_, send_msg.data(), send_msg.length());
    }
    return -1;
}

int TcpClientSock::recv_ctrl_msg(std::string &msg, int timeout)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));

    if(!glob_ctrl_msg_.empty()) {
        msg = glob_ctrl_msg_;
        //LREP("code = {}\n", get_code(msg));
    }
    return glob_ctrl_msg_.size();
}

int TcpClientSock::send_data_msg(const std::string &msg)
{
    glob_ctrl_msg_.clear();
    glob_data_msg_.clear();
    if(get_data_sock_status() == STATUS_CONNECTED) {
        return sock_send_data(datasock_, msg.data(), msg.length());
    }
    return -1;
}

int TcpClientSock::send_data_msg(const uint8_t *data, int len)
{
    glob_ctrl_msg_.clear();
    glob_data_msg_.clear();
    if(get_data_sock_status() == STATUS_CONNECTED) {
        return sock_send_data(datasock_, data, len);
    }
    return -1;
}

int TcpClientSock::recv_data_msg(std::string &msg, int timeout)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));

    if(!glob_data_msg_.empty()) {
        msg = glob_data_msg_;        
    }
    return msg.size();
}

int TcpClientSock::start_data_channel()
{
    std::string recv_msg;
    recv_msg.clear();    
    // send message passive
    if(send_ctrl_msg("pasv") <= 0) {
        WARN("pasv cmd failed\n");
        return FTP_ERR_SEND_CTRL_MSG;
    }

    // receive data channel info message
    if(recv_ctrl_msg(recv_msg, 100)) {
        WARN("{}", recv_msg);
    } else {
        WARN("start_data_channel failed\n");
        return FTP_ERR_RECV_CTRL_MSG;
    }

    std::string ip;
    int port;
    int ret = get_ip_port(recv_msg, ip, port);
    if(ret == 0) {

        ret = start_data_sock(ip, port);
        if(ret == 0) {            
            return FTP_ERR_NONE;
        } else {
            WARN("data conn failed {}\n", ret);
            return FTP_ERR_OPEN_DATA_CONN;
        }
    } else {
        WARN("no ip and port in recv msg\n");
        return FTP_ERR_PASV;
    }
}

void TcpClientSock::listen_data()
{
    int err_count = 0;
    int quit_code = 0;

    while (is_run_data_) {
        int event = datasock_->wait_event(100, true, false);
        if (event & EVENT_ERROR) {
            err_count++;
            if(err_count > 10) {
                LREP("select failed\n");
                quit_code = -1;
                break;
            }
        } else if (event & EVENT_READABLE) {
            uint8_t data[4096];
            memset(data, 0, 4096);
            ssize_t rlen = recv(datasock_->fd(), (char *)data, 4096, 0);
            if (rlen <= 0) {
                LREP("read failed\n");
                quit_code  = -2;
                break;
            } else {
                glob_data_msg_ += std::string((const char*)data, rlen);
            }
        }
    }
    LREP("close data connection run: {} code: {}\n", is_run_data_, quit_code);
    datasock_->close_connection();
    data_sock_status_ = lib::ipc::STATUS_DISCONNECTED;
}

void TcpClientSock::listen_control()
{
    int err_count = 0;
    int quit_code = 0;
    while (is_run_control_) {
        int event = ctrlsock_->wait_event(100, true, false);
        if (event & EVENT_ERROR) {
            err_count++;
            if(err_count > 10) {
                LREP("select failed\n");
                quit_code = -1;
                break;
            }
        } else if (event & EVENT_READABLE) {
            uint8_t data[4096];
            memset(data, 0, 4096);
            ssize_t rlen = recv(ctrlsock_->fd(), (char *)data, 4096, 0);            
            if (rlen <= 0) {
                LREP("read failed\n");
                quit_code  = -2;
                break;
            } else {
                glob_ctrl_msg_ += std::string((const char*)data, rlen);
            }
        }
    }
    LREP("close control connection run: {} code: {}\n", is_run_control_, quit_code);
    ctrlsock_->close_connection();
    ctrl_sock_status_ = lib::ipc::STATUS_DISCONNECTED;
}

void TcpClientSock::stop()
{
    LREP("stop sock\n");
    stop_data_sock();
    stop_ctrl_sock();
}


int TcpClientSock::get_ip_port(const std::string msg, std::string &ip, int &port)
{
    char match[100];
    regex_t reg;
    int nm = 10;
    regmatch_t pmatch[nm];
    char pattern[] = "([0-9]+,[0-9]+,[0-9]+,[0-9]+,[0-9]+,[0-9]+)";

    if(regcomp(&reg,pattern,REG_EXTENDED) < 0)
    {
        std::cerr<<"compile error."<<std::endl;
    }
    int err = regexec(&reg,msg.c_str(),nm,pmatch,0);
    if(err == REG_NOMATCH)
    {
        std::cout<<"No ip and port match in pasv message."<<std::endl;
        return -1;
    }
    if(err)
    {
        std::cout<<"pasv message regex match error."<<std::endl;
        return -1;
    }
    for(int i=0;i<nm && pmatch[i].rm_so!=-1;i++)
    {
        int len = pmatch[i].rm_eo-pmatch[i].rm_so;
        if(len)
        {
            memset(match,'\0',sizeof(match));
            memcpy(match,msg.c_str()+pmatch[i].rm_so,len);
            break;
        }
    }

    int d1,d2,d3,d4,d5,d6;
    sscanf(match,"%d,%d,%d,%d,%d,%d",&d1,&d2,&d3,&d4,&d5,&d6);
    std::stringstream ss;
    ss<<d1<<"."<<d2<<"."<<d3<<"."<<d4;
    ss>>ip;
    ss.clear();
    port = d5*256 + d6;

    return 0;
}

int TcpClientSock::get_code(const std::string &msg)
{
    int code;
    std::stringstream ss;
    ss<<msg;
    ss>>code;
    ss.clear();
    return code;
}

}

} //namespace app

