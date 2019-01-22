#include <client/ftpmanager.h>

namespace app {
namespace client {

FtpManager::FtpManager()
{

}


FtpManager::~FtpManager()
{
    stop();
}

int FtpManager::start(const std::string &iaddress, int iport,
                      const std::string &usrname, const std::string &passwd, const std::string &prefix_path)
{
    ipaddr_ = iaddress;
    port_ = iport;
    username_ = usrname;
    passwd_ = passwd;
    prefix_path_ = prefix_path;

    if(prefix_path_ != "") {
        if(!lib::string::end_with(prefix_path_, '/')) {
            prefix_path_ += "/";
        }

        if(prefix_path_.at(0) == '/') {
            prefix_path_ = std::string(&(prefix_path_.c_str()[1]));
        }
    }

    LREP("ip: {}:{} account: {}:{}\n", ipaddr_, port_, username_, passwd_);
    return 0;
}

int FtpManager::stop()
{
    LREP("stop ftp manager\n");
    return 0;
}


int FtpManager::setup_channel()
{
    if(start_ctrl_sock(ipaddr_, port_) == 0) {
        std::string recv_msg;

        // receive general info message
        if(recv_ctrl_msg(recv_msg, 100)) {
            WARN("recv: {}", recv_msg);
        } else {
            return FTP_ERR_RECV_CTRL_MSG;
        }

        // send username
        if(send_ctrl_msg("USER " + username_) <= 0) {
            return FTP_ERR_SEND_CTRL_MSG;
        }

        // receive general info message
        if(recv_ctrl_msg(recv_msg, 100)) {
            WARN("recv: {}", recv_msg);
        } else {
            return FTP_ERR_RECV_CTRL_MSG;
        }

        // send password
        if(send_ctrl_msg("PASS " + passwd_) <= 0) {
            return FTP_ERR_SEND_CTRL_MSG;
        }

        // receive general info message
        if(recv_ctrl_msg(recv_msg, 100)) {
            WARN("recv: {}", recv_msg);
        } else {
            return FTP_ERR_RECV_CTRL_MSG;
        }

        // send password
        if(send_ctrl_msg("SYST") <= 0) {
            return FTP_ERR_SEND_CTRL_MSG;
        }

        // receive general info message
        if(recv_ctrl_msg(recv_msg, 100)) {
            WARN("recv: {}", recv_msg);
        } else {
            return FTP_ERR_RECV_CTRL_MSG;
        }

        return start_data_channel();
    }

    return FTP_ERR_OPEN_CTRL_CONN;
}

int FtpManager::remote_cwd(const std::string &path, std::string &recv_msg)
{
    return remote_ctrl_send_recv("cwd " + path, recv_msg);
}

int FtpManager::remote_ls(std::string &recv_msg)
{
    return remote_data_send_recv("list", recv_msg);
}

int FtpManager::remote_pwd(std::string &recv_msg)
{
    return remote_ctrl_send_recv("pwd", recv_msg);
}

int FtpManager::remote_mkd(const std::string &path, std::string &recv_msg)
{
    return remote_ctrl_send_recv("mkd " + path, recv_msg);
}

int FtpManager::remote_ctrl_send_recv(const std::string &cmd,
                                      std::string &recv_msg)
{
    int err = FTP_ERR_NONE;
    if(get_ctrl_sock_status() != STATUS_CONNECTED) {
        err = setup_channel();
    }

    if(err == FTP_ERR_NONE) {
        int len = send_ctrl_msg(cmd);
        if(len > 0) {            
            recv_ctrl_msg(recv_msg, 100);
            //LREP("{}: {}\n",cmd , recv_msg);
        } else {
            err = FTP_ERR_RECV_CTRL_MSG;
        }
    }
    return err;
}

int FtpManager::remote_data_send_recv(const std::string &cmd, std::string &recv_msg)
{
    int err = FTP_ERR_NONE;
    if(get_ctrl_sock_status() != STATUS_CONNECTED) {
        err = setup_channel();
    }

    if(get_data_sock_status() != STATUS_CONNECTED) {
        err = start_data_channel();
    }

    if(err == FTP_ERR_NONE) {
        int len = send_ctrl_msg(cmd);
        if(len > 0) {
            recv_data_msg(recv_msg, 100);
            LREP("{}: {}\n", cmd, recv_msg);
        } else {
            err = FTP_ERR_RECV_CTRL_MSG;
        }
    }
    return err;
}

int FtpManager::remote_raw_cmd(const std::string &cmd)
{
    int err = FTP_ERR_NONE;
    if(get_ctrl_sock_status() != STATUS_CONNECTED) {
        err = setup_channel();
    }

    if(err == FTP_ERR_NONE) {
        int len = send_ctrl_msg(cmd);
        if(len > 0) {
            std::string msg = "";
            recv_ctrl_msg(msg, 100);
            LREP("{}: {}\n",cmd , msg);
        } else {
            err = FTP_ERR_RECV_CTRL_MSG;
        }
    }
    return err;
}

int FtpManager::remote_mkd_recursive(const std::string &path)
{
    std::string folder, msg;
    folder.clear();    
    int err = FTP_ERR_CD;
    std::vector<std::string> vect;
    lib::string::split(path, "/", vect);
    for(size_t i = 0; i < vect.size(); i++) {
        if(i == 0)
            folder += "/" + vect.at(i) + "/";
        else
            folder += vect.at(i) + "/";
        //LREP("folder: {} \n", folder);
        err = remote_mkd(folder, msg);
    }

    if(err == FTP_ERR_NONE) {
        err = remote_cwd(folder, msg);
        if(err == FTP_ERR_NONE &&
                msg.find("successful") != std::string::npos){
            err = FTP_ERR_NONE;
        } else {
            err = FTP_ERR_CD;
        }
    }

    return err;
}



int FtpManager::remote_put(const std::string &file_name,
                           const std::string &local_path,
                           const std::string &remote_path)
{
    int err = FTP_ERR_PUT;
    std::string msg;
    std::string full_path = local_path;
    if(!lib::string::end_with(full_path, '/')) {
        full_path += "/";
    }

    full_path += file_name;

    //LREP("filenamefull: {}\n", );
    if(!lib::filesystem::exists(full_path)) {
        ERR("file: {} not found\n", full_path);
        return FTP_ERR_FILE;
    }


    // check if connection is now opening or not
    if(get_ctrl_sock_status() != STATUS_CONNECTED) {
        err = setup_channel();
        if(err != FTP_ERR_NONE) {
            //WARN("setup channel failed err = {}\n", err);
            return err;
        }
    }

    std::string remote_path_make = prefix_path_ + remote_path;
    err = remote_cwd("/", msg);

    if(err != FTP_ERR_NONE) {
        WARN("cd to / failed err = {}\n", err);
        return err;
    }

    // create directory recursive on server
    err = remote_mkd_recursive(remote_path_make);
    if(err == FTP_ERR_NONE) {

        if(get_data_sock_status() != STATUS_CONNECTED) {
            err = start_data_channel();
        }

        if(err != FTP_ERR_NONE) {
            TcpClientSock::stop();
            return err;
        }


        remote_ctrl_send_recv("stor " + file_name, msg);
        int code = get_code(msg);
        if(code != 125 && code != 150)
        {
            WARN("recv error code {}\n", code);
            TcpClientSock::stop();
            return FTP_ERR_CODE;
        }

        std::fstream fs;
        fs.open(full_path, std::ios::in);
        if(fs.is_open() == false)
        {
            WARN("open local file error\n");
            return FTP_ERR_FILE;
        }

        char tempc;
        while(true)
        {
            if(!fs)
            {
                break;
            }
            uint8_t buf[1000];
            int count = 0;
            while(fs.get(tempc))
            {
                buf[count++] = tempc;
                if(count == 1000)
                {
                    break;
                }
            }

            if(get_data_sock_status() != STATUS_CONNECTED) {
                err = start_data_channel();
            }

            if(err != FTP_ERR_NONE) {
                ERR("start data channel while sending failed err = {}\n", err);
                TcpClientSock::stop();
                return err;
            }

            if(send_data_msg(buf, count) <= 0)
            {
                //ERR("Send data msg failed");
                TcpClientSock::stop();
                //return FTP_ERR_PUT;
                break;
            }
        }

        stop_data_sock();

        if(recv_ctrl_msg(msg, 100))
            WARN("{}", msg);
    } else {
        ERR("cd to on remote: {} failed disconnect all\n", remote_path_make);
        TcpClientSock::stop();
    }

    return err;
}


}
} //namespace app

