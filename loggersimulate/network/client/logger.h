#ifndef LOGGER_H
#define LOGGER_H



#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <regex.h>
#include <ifaddrs.h>

#include <cpp_lib/ipc.h>
#include <cpp_lib/thread.h>
#include <cpp_lib/debug.h>
#include <cpp_lib/convert.h>

#include <jsoncpp/json/json.h>
#include <cpp_lib/datalog.h>
#include <configuration.h>
#include <board/tagread.h>
#include <mutex>
#include <client/ftpmanager.h>
#include <client/realtimedata.h>

using namespace lib::ipc;

namespace app {
namespace client {

class Logger : public FtpManager
{

public:    
    Logger();
    ~Logger(){stop();}

    int start();
    void stop();


    void get_current_time();
    std::string get_current_time_cont();
    void read_save_tag_value();
    void read_save_tag_value_by_channel(const std::string &channel);
    void read_tag_value();
    void logger_create_file(const std::string &name);
    FileDesc logger_create_logfile();
    void logger_write_row(const std::vector<std::string> row);
    void logger_write_row(const std::string &x,
                          const std::string &y,
                          const std::string &z);
    void logger_close();
    void thread_function();
    void thread_background();
    void directory_manager(const std::string &dir, int maxNumberOfFiles);

    std::string pwd();
    void load_buff_file(const std::string &filename);
    void add_buff_point(const FileDesc &file);
    void rem_buff_first();
    bool get_buff_first(FileDesc &point);
    int  get_buff_size() const {return root_.size();}
    void save_buff_file();
    void set_push_file_all_now();
    void set_push_file_group_now(int group_id);
    void set_push_file_channel_now(std::string channel);

private:
    std::shared_ptr<tagmanager> tag_man_;

    std::tm *time_now_;
    std::shared_ptr<lib::datalog::CSVFile> loader_;
    std::string local_folder_path_;

    std::thread listen_thread_;
    volatile bool listen_run_;    
    std::thread backgnd_thread_;
    volatile bool backgnd_run_;

    app::user_config config_;    
    std::mutex mutex_;
    std::string file_name_;
    Json::Value root_;
    int                 max_num_file_;
    bool                push_now_all_;
    bool                push_now_channel_;
    std::string         push_channel_id_;
};




}
}
#endif // FTPCLIENT_H
