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



    //std::string ip, netmask, broadcast;
    //app::server::TcpClient::get_if_ipaddress("enp7s0", ip, netmask, broadcast);

    //LREP("ipaddress {} netmask {} broadcast {}\n", ip, netmask, broadcast);

    LREP("curr path = {}\r\n", lib::filesystem::current_path().string());

//    std::string test = "";
//    std::string last = std::string((char*)(&test.back()));
//    LREP("last is: {} \n", last);

    std::string line;
    std::vector<std::string> vect;
    while(keep_running) {
        std::this_thread::sleep_for(1_s);
        /*
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
        */


    }



//    server->stop();
//    app::userconfig::instance()->close_file();

    return 0;
}

