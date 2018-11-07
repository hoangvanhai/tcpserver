#ifndef CONTROL_CLIENT_H
#define CONTROL_CLIENT_H

#include <client/tcpclient.h>
#include <client/logger.h>

using namespace lib::ipc;

namespace app {
namespace client {

#define USERNAME_LENGTH_MAX         10
#define PASSWORD_LENGTH_MAX         10

#define SER_GET_INPUT_ALL           0x0001
#define SER_GET_INPUT_GROUP         0x0002
#define SER_GET_INPUT_CHAN          0x0003
#define SER_SET_SAMPLE_START        0x0011
#define LOG_REQ_CALIB_START         0x0021
#define SER_SET_CALIB_START         0x0022
#define LOGGER_LOGGING_IN           0x0100
#define SER_LOGGING_STATUS          0x0101
#define LOGGER_LOGGING_OUT          0x0102


#define GET_MSG_TYPE(data)          ((data)[0] << 8 | (data)[1])


class ControlClient : public TcpClient
{
    enum WaitCode{
        CTRL_WAIT_NONE = 0,
        CTRL_WAIT_LOGGING = 0x01 << 1,
        CTRL_WAIT_CALIB_RESPONSE = 0x01 << 2,
    };

public:
    ControlClient();

    int start(const std::string &address,
              int port,
              const std::string &username,
              const std::string &passswd);

    int logging_in();
    int logging_out();
    int do_get_input_all();
    int do_get_input_group(int group);
    int do_get_input_channel(std::string channel);
    int do_set_calibration(int channel);
    int do_set_get_sample(int channel);


private:
    std::string user_name_;
    std::string password_;
    std::string address_;
    int         port_;
    bool        logged_in_;

    std::shared_ptr<Logger>     logger;
};




}
}
#endif // CONTROL_CLIENT_H
