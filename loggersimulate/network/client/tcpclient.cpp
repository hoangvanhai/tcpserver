#include <client/tcpclient.h>

namespace app {
namespace client {
TcpClient::TcpClient() :
    run_(false),
    notify_(nullptr),
    data_handler_(nullptr),
    config_(nullptr)
{
    csock_ = std::make_shared<lib::ipc::tcp::client>();
    status_ = STATUS_DISCONNECTED;
}


TcpClient::~TcpClient()
{
    stop();
}


int TcpClient::start(const std::string &address, int port)
{    
    int err = 0;
    ipaddress_ = address;
    ipaddress_port_ = port;
    create_thread();
    return (err);
}


bool TcpClient::stop()
{
    run_ = false;
    if(thread_listener_.joinable())
        thread_listener_.join();
    if(csock_)
        csock_->close_connection();

    LREP("disconnected\r\n");
    return true;
}

int TcpClient::send_data(const void *data, int len)
{
    int err = -1;
    if(data == NULL || len <= 0) {
        return err;
    }

    if(status_ != lib::ipc::STATUS_CONNECTED) {
        return -2;
    }

    int event = csock_->wait_event(100, false, true);

    if(event & EVENT_WRITEABLE) {
        std::shared_ptr<packet> pkg = std::make_shared<packet>(data, len);
        err = (int)csock_->write_data(pkg);
    }

    return err;
}

int TcpClient::send_data_wlen(const void *data, int len)
{
    uint8_t *send_data_ = new uint8_t[len + 4];
    int err;

    send_data_[0] = uint8_t((len >> 24) & 0xFF);
    send_data_[1] = uint8_t((len >> 16) & 0xFF);
    send_data_[2] = uint8_t((len >> 8) & 0xFF);
    send_data_[3] = uint8_t((len) & 0xFF);

    memcpy(&send_data_[4], data, len);
    err = send_data(send_data_, len + 4);
    delete[] send_data_;

    return err;
}

void TcpClient::create_thread()
{
    run_ = true;
    thread_listener_ = std::thread( [&](){
        listen_function();
    });    
}


void TcpClient::listen_function()
{
    int err = -1;
    while(run_) {

        if(config_ == nullptr) {
            config_ = std::make_shared<lib::ipc::tcp::client::configuration>(ipaddress_, ipaddress_port_);
        } else {
            config_->set_address_port(ipaddress_, ipaddress_port_);
        }

        if(config_) {
            err = csock_->open_connection(config_);
            if(err == 0) {
                status_ = STATUS_CONNECTED;
                if(notify_)
                    notify_(status_);
            } else {
                WARN("try connect to server: {}:{} err = {}\n", ipaddress_, ipaddress_port_, err);
                std::this_thread::sleep_for(2_s);
                continue;
            }
        } else {
            break;
        }

        while (run_) {
            int event = csock_->wait_event(100, true, false);
            if (event & EVENT_ERROR) {
                LREP("Select failed");
                break;
            } else if (event & EVENT_READABLE) {
                std::ssize_t rlen = csock_->read_data([&](const std::shared_ptr<output> &ipkg) {
                    std::shared_ptr<packet> pkg = std::dynamic_pointer_cast<packet>(ipkg);
                    if (!pkg) {
                        return lib::ipc::READ_ACTION_BREAK;
                    } else {
                        if(pkg->data_size() > 0) {
                            process_raw_data(pkg->data(), pkg->data_size());
                        }
                    }
                    return lib::ipc::READ_ACTION_CONTINUE;
                });

                if (rlen <= 0) {
                    LREP("read failed\n");
                    break;
                }
            }
        }
        csock_->close_connection();
        status_ = lib::ipc::STATUS_DISCONNECTED;
        if(notify_)
            notify_(status_);
    }
    csock_->close_connection();
    status_ = lib::ipc::STATUS_DISCONNECTED;
    if(notify_)
        notify_(status_);
}

void TcpClient::process_raw_data(const void *data, int len)
{
    LREP("read raw data leng {}\n", len);
    uint8_t *dataptr = (uint8_t*)data;
    if(data_handler_)
        data_handler_(&dataptr[4], len - 4);
}


}

} //namespace app

