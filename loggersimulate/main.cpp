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
#include <client/controlclient.h>


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


//    std::shared_ptr<app::server::TcpServer> server =
//            std::make_shared<app::server::TcpServer>();

//    server->start("0.0.0.0", 12345);

//    std::shared_ptr<app::client::TcpClient> tcpclient =
//            std::make_shared<app::client::TcpClient>();

    std::shared_ptr<app::client::ControlClient> controlclient =
            std::make_shared<app::client::ControlClient>();

    controlclient->start(cfg_.server.address, 1186, "user", "1234");

    std::string line;
    std::vector<std::string> vect;
    while(keep_running) {

        std::cout << "cmd>";
        getline(std::cin, line);

        if(line != "") {

            lib::string::split(line, " ", vect);            

            if(line == "q")
                break;
        }
    }



//    server->stop();
//    app::userconfig::instance()->close_file();

    return 0;
}

