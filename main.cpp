#include <iostream>
#include <server/tcpserver.h>
#include <cpp_lib/utilities.h>
#include <cpp_lib/datalog.h>
#include <cpp_lib/algorithm.h>
#include <configuration.h>
#include <client/tcpclient.h>
#include <client/ftpmanager.h>
#include <client/logger.h>
#include <cpp_lib/debug.h>
#include <sys/signal.h>
#include <usermanager.h>


using namespace std;

static bool keep_running = true;

void sigal_handler(int signal) {
    LREP("catch signal {}\n", signal);
    keep_running = false;
}


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    std::string path = "conf.d/";

    if(path.data()[path.size() - 1] == '/') {
        std::cout << "found\r\n";
    } else {
        std::cout << "not found\r\n";
    }


    std::string logdir = "log.d", logfile="logger", cfgdir = "";

    for(int i = 1; i < argc; i++){

        if(strcmp(argv[i], "--kill-all") == 0){
            //AppUtils::killAllInstance(argv[0]);
            //return 0;
        }

        if(strstr(argv[i], "--pid-file=") == argv[i]){
            const std::string fileName = std::string(&argv[i][11]);
            std::ofstream file(fileName, std::ios::out | std::ios::trunc);
            if(file.good()){
                file<<getpid();
                file.close();
            }
        }
        if(strstr(argv[i], "--log-dir=") == argv[i]){
            logdir = std::string(&argv[i][10]);
            if(!lib::string::end_with(logdir, '/')) {
                logdir += "/";
            }
        }

        if(strstr(argv[i], "--cfg-dir=") == argv[i]){
            cfgdir = std::string(&argv[i][10]);
            if(!lib::string::end_with(cfgdir, '/')) {
                cfgdir += "/";
            }
        }
    }





    struct sigaction act;
    act.sa_handler = sigal_handler;
    sigaction(SIGINT, &act, NULL);


    srand(time(NULL));    

    lib::debugger::config cfg(lib::debugger::CHANNEL_STDOUT | lib::debugger::CHANNEL_FILE);
    cfg.setup_logfile(logdir, logfile, 1024 * 1024, 3);
    lib::debugger::instance()->configure(cfg);

    app::userconfig::instance()->start(cfgdir);

    app::user_config cfg_ = app::userconfig::instance()->get_user_config();

    std::shared_ptr<app::client::Logger> logger =
            std::make_shared<app::client::Logger>();

    logger->start();


    std::shared_ptr<app::server::TcpServer> server =
            std::make_shared<app::server::TcpServer>();

    server->start("0.0.0.0", 6025);

    std::shared_ptr<app::client::TcpClient> tcpclient =
            std::make_shared<app::client::TcpClient>();



    std::string ip, netmask, broadcast;
    app::server::TcpClient::get_if_ipaddress("enp7s0", ip, netmask, broadcast);

    LREP("ipaddress {} netmask {} broadcast {}\n", ip, netmask, broadcast);

//    std::string test = "";
//    std::string last = std::string((char*)(&test.back()));
//    LREP("last is: {} \n", last);

    std::string line;
    std::vector<std::string> vect;
    while(keep_running) {

        std::cout << "cmd>";
        getline(std::cin, line);

        if(line != "") {

            lib::string::split(line, " ", vect);

            if(vect.at(0) == "start") {
                if(tcpclient->start("127.0.0.1", 12345)) {
                    tcpclient->set_recv_event([&](const void* data, int len) {
                        char *sdata = (char*)data;
                        Json::Reader reader;
                        Json::Value value;
                        reader.parse((const char*)data, (const char*)(sdata + len - 1), value);
                        std::cout << value << std::endl;
                    });
                }

            } else if(vect.at(0) == "login") {
                if(vect.size() >= 3) {
                    Json::Value root, data;
                    root["type"] = "login";
                    data["username"] = vect.at(1);
                    data["password"] = vect.at(2);
                    root["data"] = data;
                    std::cout << root << std::endl;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    tcpclient->send_data(msg.c_str(), (int)msg.size());
                }

            } else if(vect.at(0) == "logout") {
                Json::Value root;
                root["type"] = "logout";
                std::cout << root << std::endl;
                Json::FastWriter fast_writer;
                std::string msg = fast_writer.write(root);
                tcpclient->send_data(msg.c_str(), (int)msg.size());
            } else if(vect.at(0) == "adduser") {
                if(vect.size() >= 4) {
                    Json::Value root, data;
                    root["type"] = "add_user";
                    data["username"] = vect.at(1);
                    data["password"] = vect.at(2);
                    data["role"] = vect.at(3);
                    root["data"] = data;
                    std::cout << root << std::endl;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    tcpclient->send_data(msg.c_str(), (int)msg.size());
                }
            } else if(vect.at(0) == "deluser") {
                if(vect.size() >= 2) {
                    Json::Value root, data;
                    root["type"] = "del_user";
                    data["username"] = vect.at(1);
                    root["data"] = data;
                    std::cout << root << std::endl;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    tcpclient->send_data(msg.c_str(), (int)msg.size());
                }
            } else if(vect.at(0) == "changepw") { //change pw for current user
                if(vect.size() >= 3) {
                    Json::Value root, data;
                    root["type"] = "change_password";
                    data["oldpassword"] = vect.at(1);
                    data["newpassword"] = vect.at(2);
                    root["data"] = data;
                    std::cout << root << std::endl;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    tcpclient->send_data(msg.c_str(), (int)msg.size());
                }
            } else if(vect.at(0) == "reset") {
                if(vect.size() >= 2) {
                    Json::Value root, data;
                    root["type"] = "reset_password";
                    data["username"] = vect.at(1);
                    root["data"] = data;
                    std::cout << root << std::endl;
                    Json::FastWriter fast_writer;
                    std::string msg = fast_writer.write(root);
                    tcpclient->send_data(msg.c_str(), (int)msg.size());
                }
            } else if(vect.at(0) == "list") {
                UsersLogin::instance()->listAllAccount();
            }

            if(line == "q")
                break;
        }
    }



//    server->stop();
//    app::userconfig::instance()->close_file();

    return 0;
}


/*
            if(value == "adduser") {
                std::cout << "user: ";
                getline(std::cin, username);
                std::cout << "passwd: ";
                getline(std::cin, passwd);
                std::cout << "role: ";
                getline(std::cin, role);
                if(UsersLogin::instance()->addUser(username, passwd, role)) {
                    std::cout << "add user success" << std::endl;
                } else {
                    std::cout << "add user failed" << std::endl;
                }
            } else if (value == "deluser") {
                std::cout << "user: ";
                getline(std::cin, username);
                if(UsersLogin::instance()->delUser(username)) {
                    std::cout << "del user success" << std::endl;
                } else {
                    std::cout << "del user failed" << std::endl;
                }
            } else if(value == "changepw") {
                std::cout << "user: ";
                getline(std::cin, username);
                std::cout << "passwd: ";
                getline(std::cin, passwd);
                if(UsersLogin::instance()->changePasswd(username, passwd)) {
                    std::cout << "change passwd user success" << std::endl;
                } else {
                    std::cout << "change passwd user failed" << std::endl;
                }
            } else if(value == "resetpw") {
                std::cout << "user: ";
                getline(std::cin, username);
                if(UsersLogin::instance()->resetPasswd(username)) {
                    std::cout << "reset passwd for "<< username << "success" << std::endl;
                } else {
                    std::cout << "reset passwd for "<< username << "failed" << std::endl;
                }
            } else if(value == "login") {
                std::cout << "user: ";
                getline(std::cin, username);
                std::cout << "passwd: ";
                getline(std::cin, passwd);
                if(UsersLogin::instance()->checkUserPasswdMatch(username, passwd, role)) {
                    std::cout << "login success: username: " << username << " role: " << role << std::endl;
                } else {
                    std::cout << "login failed" << std::endl;
                }
            } else if(value == "list") {
                if(UsersLogin::instance()->listAllAccount()) {
                    std::cout << "list successful" << std::endl;
                } else {
                    std::cout << "list failed" << std::endl;
                }
            } else {
                std::cout << "not handle command " << value << std::endl;
            }


            if(value == "filename") {
                app::user_config cfg_ = app::userconfig::instance()->get_user_config();
                std::cout << "tentinh: ";
                getline(std::cin, temp);
                cfg_.filename.tentinh = temp;
                std::cout << "tencoso: ";
                getline(std::cin, temp);
                cfg_.filename.tencoso = temp;
                std::cout << "tentram: ";
                getline(std::cin, temp);
                cfg_.filename.tentram = temp;
                app::userconfig::instance()->save_user_config(cfg_);

            } else if (value == "deluser") {
                app::user_config cfg_ = app::userconfig::instance()->get_user_config();
                std::cout << "ip: ";
                getline(std::cin, temp);
                cfg_.server.address = temp;
                std::cout << "username: ";
                getline(std::cin, temp);
                cfg_.server.username = temp;
                std::cout << "password: ";
                getline(std::cin, temp);
                cfg_.server.passwd = temp;
                app::userconfig::instance()->save_user_config(cfg_);
            } else if(value == "connect") {
                tcpclient->start("192.168.40.119", 21);
            } else if(value == "disconnect") {
                tcpclient->stop();
            }


            lib::string::split(value, " ", vect);

            if(vect.at(0) == "ls") {
                ftp->remote_ls(temp);
            } else if(vect.at(0) == "cd") {
                if(vect.size() >= 2)
                    ftp->remote_cwd(vect.at(1), temp);
            } else if(vect.at(0) == "pwd") {
                ftp->remote_pwd(temp);
            } else if(vect.at(0) == "mkd") {
                if(vect.size() >= 2)
                    ftp->remote_mkd(vect.at(1), temp);
            } else if(vect.at(0) == "folder") {
                if(vect.size() >= 2) {
                    if(ftp->remote_mkd_recursive(vect.at(1)) == 0) {
                        LREP("cd ok\n");
                    } else {
                        LREP("cd err \n");
                    }
                }
            } else if(vect.at(0) == "put") {
                int i = 0;

                Json::StyledStreamWriter writer;
                Json::Value root;
                for(i = 0; i < 10; i++){
                    std::ofstream outfile;
                    std::string filename = "./test" + std::to_string(i);
                    outfile.open(filename, std::ios::out);
                    if(outfile.is_open()) {
                        root["file"] = "file" + std::to_string(i);
                        root["data"] = "data";
                        writer.write(outfile,root);
                        outfile.close();
                    } else {
                        LREP("can't open file {}\n", filename);
                    }

                    ftp->remote_put(filename,
                                    "./",
                                    "2018/10/09");
                    std::this_thread::sleep_for(2_s);
                }
            }
*/
