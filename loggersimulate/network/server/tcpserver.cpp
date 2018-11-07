#include "tcpserver.h"
#include <cpp_lib/typedef.h>
namespace app {
namespace server {

TcpServer::TcpServer() :
    run_background(false),
    run_listener(false)
{    

    folder_path_ = "data/";
    ssock = std::make_shared<lib::ipc::tcp::server>();

    int err = mkdir(folder_path_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (err == -1)
    {
        if(errno == EEXIST ) {
            std::cout << "folder existed\n";
        } else {
            std::cout << "cannot create session namefolder error:" <<
                         strerror(errno) << std::endl;
        }
    } else {
        std::cout << "create new folder " << folder_path_ << std::endl;
    }

}

TcpServer::~TcpServer()
{
    stop();
}


void TcpServer::listen_thread_func()
{
    int err_count = 0;
    while(run_listener) {
        int event = ssock->wait_event(100, true, false);
        if(event & EVENT_ERROR) {
            err_count++;
            if(err_count > 0) {
                LREP("wait failed\n");
                break;
            }
        } else if (event & EVENT_READABLE) {
            std::ssize_t rlen = ssock->read_data([&](const std::shared_ptr<output> &args) {
                std::shared_ptr<lib::ipc::tcp::client> csock =
                    std::dynamic_pointer_cast<lib::ipc::tcp::client>(args);
                if(!csock) {
                    LREP("cast failed\n");
                    return lib::ipc::READ_ACTION_BREAK;
                } else {
                    create_newconnection(csock);
                    return lib::ipc::READ_ACTION_CONTINUE;
                }
            });

            if (rlen <= 0) {
                LREP("read failed\n");
                break;
            }
        }
    }
    ssock->close_connection();

}

void TcpServer::background_thread_func()
{
    while(run_background) {
        //RAW(".");
        std::this_thread::sleep_for(1000_ms);
        std::list<std::shared_ptr<TcpClient>>::iterator i = listClient.begin();
        while(i != listClient.end()){
            std::shared_ptr<TcpClient> client = *i;
            if(client->get_status() != lib::ipc::STATUS_CONNECTED){
                client.reset();
                LREP("remove client point\n");
                i = listClient.erase(i);
                continue;
            }
            ++i;
        }



    }
}

void TcpServer::create_thread()
{
    run_listener = true;
    thread_listener = std::thread([&]() {
        listen_thread_func();
    });

    run_background = true;
    thread_background= std::thread([&]() {
        background_thread_func();
    });
}

void TcpServer::send_all_client(const void *data, int len)
{
    for(auto client : listClient){
        client->send_data(data, len);
    }
}

void TcpServer::create_newconnection(std::shared_ptr<endpoint> sock)
{
    LREP("accept new connection: {}:{}\n",
         std::dynamic_pointer_cast<lib::ipc::tcp::client>(sock)->address(),
         std::dynamic_pointer_cast<lib::ipc::tcp::client>(sock)->port());

    std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(sock);
    if(!client->start()) {
        LREP("run connection failed\n");
        return;
    }
    client->set_data_handler([&](const void *data, int len) {
        if(data_handler_) {
            data_handler_(data, len);
        }
    });

    listClient.push_back(client);
}

bool TcpServer::start(const std::string &address, int port)
{
    int err = -1;
    if(!config) {
        config = std::make_shared<lib::ipc::tcp::server::configuration>(address, port);
    }

    if(config) {
        config->set_address_port(address, port);
        config->set_no_delay(true);
        config->set_rx_buffer_size(1024 * 1024);
        config->set_tx_buffer_size(1024 * 1024);
        err = ssock->open_connection(config);
        if(!err) {
            create_thread();
            LREP("server started on {}:{}\n",address, port);
        }

    }
    return (err == 0);
}

void TcpServer::stop()
{
    run_listener = false;
    if(thread_listener.joinable()) thread_listener.join();

    std::list<std::shared_ptr<TcpClient>>::iterator i = listClient.begin();
    while(i != listClient.end()){
        std::shared_ptr<TcpClient> client = *i;
            client.reset();
            i = listClient.erase(i);
    }

    run_background = false;
    if(thread_background.joinable()) thread_background.join();
}



}
}



