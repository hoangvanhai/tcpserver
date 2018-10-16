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

bool TcpClient::start()
{
    int err = -1;
    if(config_) {
        err = csock_->open_connection(config_);
        if(err == 0) {
            create_thread();
        }
    }
    return (err == 0);
}

bool TcpClient::start(std::shared_ptr<tcp::client::configuration> iconf)
{
    int err = -1;
    if(iconf) {
        if(!config_) {
            config_ = std::make_shared<lib::ipc::tcp::client::configuration>(iconf->address(), iconf->port());
        } else {
            config_->set_address_port(iconf->address(), iconf->port());
        }

        err = csock_->open_connection(config_);
        if(err == 0) {
            create_thread();
        }
    }
    return (err == 0);
}

bool TcpClient::start(const std::string &address, int port)
{    
    int err = -1;
    if(config_ == nullptr) {
        config_ = std::make_shared<lib::ipc::tcp::client::configuration>(address, port);
    } else {
        config_->set_address_port(address, port);
    }

    if(config_) {
        err = csock_->open_connection(config_);
        if(err == 0) {
            create_thread();
        }
    }
    return (err == 0);
}

/**
 * @brief TcpClient::Configuration
 * @param iconf
 * @return
 */
void TcpClient::configuration(std::shared_ptr<tcp::client::configuration> iconf)
{
    if(!config_) {
        config_ = std::make_shared<lib::ipc::tcp::client::configuration>(iconf->address(), iconf->port());
        config_->set_no_delay(iconf->no_delay());
        config_->set_rx_buffer_size(iconf->rx_buffer_size());
        config_->set_tx_buffer_size(iconf->tx_buffer_size());
    } else {
        config_->set_address_port(iconf->address(), iconf->port());
        config_->set_no_delay(iconf->no_delay());
        config_->set_rx_buffer_size(iconf->rx_buffer_size());
        config_->set_tx_buffer_size(iconf->tx_buffer_size());
    }
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

void TcpClient::create_thread()
{
    run_ = true;
    thread_listener_ = std::thread( [&](){
        listen_function();
    });
    status_ = lib::ipc::STATUS_CONNECTED;
}


void TcpClient::listen_function()
{
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
}

void TcpClient::process_raw_data(const void *data, int len)
{
    LREP("read raw data leng {}\n", len);

    if(data_handler_)
        data_handler_(data, len);
}


}

} //namespace app

