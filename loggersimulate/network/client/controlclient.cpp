#include <client/controlclient.h>

namespace app {
namespace client {

ControlClient::ControlClient()
{

}

int ControlClient::start(const std::string &address,
                         int port,
                         const std::string &username,
                         const std::string &passswd)
{

    user_name_ = username;
    password_ = passswd;
    address_ = address;
    port_ = port;

    int err = TcpClient::start(address_, port_);

    if(err == 0) {
        set_notify([&](lib::ipc::STATUS stat) {
            switch(stat) {
            case STATUS_CONNECTED:
                if(!logged_in_) {
                    WARN("try logging in ret: {}\n", logging_in());

                }
                break;

            case STATUS_DISCONNECTED:
                    logged_in_ = false;
                break;

            case STATUS_NWK_DOWN:

                break;

            case STATUS_CONNECTING:

                break;

            default:
                break;
            }
        });

        set_recv_event([&](const void *data, int len) {
            LREP("data len: {}\n", len);
            if(len <= 0) {
                ERR("len = {}\n", len);
                return;
            }

            uint8_t *pdata = (uint8_t*)data;
            switch(GET_MSG_TYPE(pdata)) {
                 case SER_GET_INPUT_ALL:
                    do_get_input_all();
                break;
                 case SER_GET_INPUT_GROUP:
                    do_get_input_group(pdata[4]);
                break;
                 case SER_GET_INPUT_CHAN:
                    do_get_input_channel(std::string((const char*)&pdata[4], pdata[3]));
                break;
                 case SER_SET_SAMPLE_START:
                    do_set_get_sample(pdata[4]);
                break;
                 case LOG_REQ_CALIB_START:
                    do_set_calibration(pdata[4]);
                break;
                 case SER_SET_CALIB_START:

                break;
                 case LOGGER_LOGGING_IN:

                break;
                 case SER_LOGGING_STATUS:
                if(pdata[4] == 0x00) {
                    logged_in_ = true;
                } else {
                    logged_in_ = false;
                }
                break;
                 case LOGGER_LOGGING_OUT:
                    logged_in_ = false;
                break;
            default:
                WARN("unhandled command {}\n", GET_MSG_TYPE(pdata));
                break;
            }

        });

    }

    logger = std::make_shared<Logger>();

    logger->start();

    return err;
}

int ControlClient::logging_in()
{
    uint8_t data[USERNAME_LENGTH_MAX + PASSWORD_LENGTH_MAX + 3];
    memset(data, 0, USERNAME_LENGTH_MAX + PASSWORD_LENGTH_MAX + 2);
    data[0] = uint8_t(LOGGER_LOGGING_IN >> 8 & 0xFF);
    data[1] = uint8_t(LOGGER_LOGGING_IN & 0xFF);
    data[2] = uint8_t(USERNAME_LENGTH_MAX + PASSWORD_LENGTH_MAX);
    int len = user_name_.length() > USERNAME_LENGTH_MAX ? USERNAME_LENGTH_MAX : user_name_.length();
    memcpy(&data[3], user_name_.data(), len);
    len = password_.length() > PASSWORD_LENGTH_MAX ? PASSWORD_LENGTH_MAX : password_.length();
    memcpy(&data[USERNAME_LENGTH_MAX + 3], password_.data(), len);
    return send_data_wlen(data, USERNAME_LENGTH_MAX + PASSWORD_LENGTH_MAX + 3);
}

int ControlClient::logging_out()
{
    uint8_t data[3];
    data[0] = uint8_t(LOGGER_LOGGING_OUT >> 8 & 0xFF);
    data[1] = uint8_t(LOGGER_LOGGING_OUT & 0xFF);
    data[2] = 0;
    return send_data_wlen(data, 3);
    return 0;
}

int ControlClient::do_get_input_all()
{
    logger->set_push_file_all_now();
    return 0;
}

int ControlClient::do_get_input_group(int group)
{
    logger->set_push_file_group_now(group);
    return 0;
}

int ControlClient::do_get_input_channel(std::string channel)
{
    logger->set_push_file_channel_now(channel);
    LREP("request log channel: {}\r\n", channel);
    return 0;
}

int ControlClient::do_set_calibration(int channel)
{
    return 0;
}

int ControlClient::do_set_get_sample(int channel)
{
    return 0;
}



}

} //namespace app

