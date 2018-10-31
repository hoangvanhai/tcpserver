
#include <client/logger.h>
#include <cpp_lib/debug.h>
#include <cpp_lib/typedef.h>
#include <cpp_lib/rand.h>

namespace app {
namespace client {

/**
 * @brief FtpClient::FtpClient
 * @param address
 * @param port
 */

Logger::Logger() {
    listen_run_ = false;
    loader_ = nullptr;    
}

int Logger::start()
{    
    config_ = app::userconfig::instance()->get_user_config();
    std::string cwd = pwd();
    cwd = lib::string::trim(cwd, '\r');
    cwd = lib::string::trim(cwd, '\n');
    if(!lib::string::end_with(cwd, '/')) {
        cwd += "/";
    }

    LREP("pwd = {}\n", cwd);
    max_num_file_ = config_.server.max_hold;
    local_folder_path_ = config_.server.data_path;
    LREP("local_folder_path_ = {}\n", local_folder_path_);

    if(lib::filesystem::create_directories(local_folder_path_)) {
        LREP("create local path success\n");
    }

    app::user_config cfg = app::userconfig::instance()->get_user_config();
    tag_man_ = std::make_shared<tagmanager>(APP_SYSTEM_MAX_TAG);
    for(int i = 0; i < APP_SYSTEM_MAX_TAG; i++) {
        tag_man_->add_tag(cfg.tag[i]);
        // Only add AI point
        if(cfg.tag[i].hw_name.find("BoardIO:AI") != std::string::npos) {
            app::RealtimeData::instance()->add_point_value(cfg.tag[i]);
        }
    }

    FtpManager::start(config_.server.address, 21/*config_.server.port*/,
          config_.server.username, config_.server.passwd);

    LREP("tag size: {}\n", tag_man_->tag_list_.size());

    listen_run_ = true;
    listen_thread_ = std::thread([&](){
        thread_function();
    });

    backgnd_run_ = true;
    backgnd_thread_ = std::thread([&](){
        thread_background();
    });


    load_buff_file("buffer.json");

    return 0;
}

void Logger::stop()
{
    LREP("stop logger\n");
    listen_run_ = false;
    if(listen_thread_.joinable())
        listen_thread_.join();

    backgnd_run_ = false;
    if(backgnd_thread_.joinable())
        backgnd_thread_.join();
}


void Logger::get_current_time()
{
    std::time_t t = std::time(0);
    time_now_ = std::localtime(&t);
}

std::string Logger::get_current_time_cont()
{

    std::string time;
    get_current_time();
    std::string year, month, mday, hour, min, sec;
    year.resize(4);month.resize(2);mday.resize(2);hour.resize(2);min.resize(2);sec.resize(2);

    sprintf((char*)year.data(), "%04d", time_now_->tm_year + 1900);
    sprintf((char*)month.data(), "%02d", time_now_->tm_mon + 1);
    sprintf((char*)mday.data(), "%02d", time_now_->tm_mday);
    sprintf((char*)hour.data(), "%02d", time_now_->tm_hour);
    sprintf((char*)min.data(), "%02d", time_now_->tm_min);
    sprintf((char*)sec.data(), "%02d", time_now_->tm_sec);

    time = year + month + mday + hour + min + sec;

    return time;

}

void Logger::read_tag_value()
{
    //LREP("\r\nread tag: ");
    tag_man_->scan_all_raw_inter_avg_value();

    for(auto &var : tag_man_->tag_list_) {
        if(var->get_hw_name().find("BoardIO:AI") != std::string::npos &&
                var->get_tag_enable()) {
            //LREP("process:{}\n", var->get_hw_name(), var->get_usr_name());
            double value_calib, value_error;
            double value;
            std::string status = "00";
            if(var->has_pin_calib() && var->has_pin_error())
            {
                if(!tag_man_->get_raw_value_by_hwname(
                            var->get_tag_pin_calib(),
                            value)) {
                    return;
                }
                value_calib = value;
                if(!tag_man_->get_raw_value_by_hwname(
                            var->get_tag_pin_error(),
                            value)) {
                    return;
                }
                value_error = value;


                if(value_error > 0) {
                    status = "02";
                } else {
                    if(value_calib > 0) {
                        status = "01";
                    } else {
                        status = "00";
                    }
                }
                //LREP("status = {}\n", status);
            } else {
                //LREP("no calib - error pin\n");
            }

            double final_value = 0, inter_value = 0;
            int cal_type = var->get_final_cal_type();
            bool cal_revert = var->get_final_cal_revert();
            switch(cal_type) {
            case 0:
                inter_value = var->get_inter_value_avg_stream();
                final_value = inter_value;
                break;
            case 1: {
                    inter_value = var->get_inter_value_avg_stream();
                    double o2_comp = var->get_o2_comp();
                    double o2_coeff = 1;
                    double get_value;
                    if(tag_man_->get_inter_value_avg_stream_by_hwname(
                                var->get_o2_comp_hw_name(),
                                get_value)) {
                        double den = 20.9 - get_value;
                        if((20.9 - get_value) != 0  && (20.9 - o2_comp) != 0) {
                            o2_coeff =  ((20.9 - o2_comp) / den);
                        }
                    }
                    if(!cal_revert)
                        final_value = o2_coeff * inter_value;
                    else
                        final_value = inter_value / o2_coeff;
                }
                break;
            case 2: {
                inter_value = var->get_inter_value_avg_stream();
                double press_comp = var->get_press_comp();
                double temp_comp = var->get_temp_comp();
                double localvalue, temp_coeff = 1, press_coeff = 1;
                if(tag_man_->get_inter_value_avg_stream_by_hwname(
                            var->get_press_comp_hw_name(),
                            localvalue)) {
                    if(localvalue != 0 && press_comp != 0)
                        press_coeff = press_comp / localvalue;
                }

                if(tag_man_->get_inter_value_avg_stream_by_hwname(
                            var->get_temp_comp_hw_name(),
                            localvalue)) {
                    if(((273 + localvalue) != 0) && ((273 + temp_comp) != 0))
                        temp_coeff = (273 + localvalue) / (273 + temp_comp);
                }

                if(!cal_revert)
                    final_value = inter_value * temp_coeff * press_coeff;
                else
                    final_value = inter_value / (temp_coeff * press_coeff);
            }
                break;
            default:
                break;
            }

            if(inter_value < 0) inter_value = 0;
            if(final_value < 0) final_value = 0;


            //LREP("cal type: {} final: {} \n", cal_type, final_value);

            app::RealtimeData::instance()->set_all_value(
                        var->get_hw_name(),
                        final_value, inter_value, status);

        } else {
        }
    }


}


void Logger::read_save_tag_value() {      

    for(auto &var : tag_man_->tag_list_) {
        if(var->get_hw_name().find("BoardIO:AI") != std::string::npos &&
                var->get_tag_report()) {
            //LREP("process:{}\n", var->get_hw_name(), var->get_usr_name());
            std::vector<std::string> vect;
            double value_calib, value_error;
            double value;
            std::string status = "00";
            if(var->has_pin_calib() && var->has_pin_error())
            {
                if(!tag_man_->get_raw_value_by_hwname(
                            var->get_tag_pin_calib(),
                            value)) {
                    return;
                }
                value_calib = value;
                if(!tag_man_->get_raw_value_by_hwname(
                            var->get_tag_pin_error(),
                            value)) {
                    return;
                }
                value_error = value;


                if(value_error > 0) {
                    status = "\t02";
                } else {
                    if(value_calib > 0) {
                        status = "\t01";
                    } else {
                        status = "\t00";
                    }
                }
            }

            std::string tag_name, tag_value, tag_unit, tag_time;
            double final_value, inter_value;
            switch(var->get_final_cal_type()) {
            case 0:
                tag_unit = "\t" + var->get_tag_inter_unit();
                inter_value = var->get_inter_value_avg_report();
                final_value = inter_value;
                WARN("{} NO COMP: inter value = {}, final value = {}\r\n", var->get_usr_name(), inter_value, final_value);
                break;
            case 1: {
                    tag_unit = "\t" + var->get_tag_final_unit();
                    inter_value = var->get_inter_value_avg_report();
                    double o2_comp = var->get_o2_comp();
                    double get_value;
                    double o2_coeff = 1;
                    if(tag_man_->get_inter_value_avg_report_by_hwname(
                                var->get_o2_comp_hw_name(),
                                get_value)) {
                        double den = 20.9 - get_value;
                        if(den != 0 && (20.9 - o2_comp) != 0) {
                            o2_coeff = ((20.9 - o2_comp) / den);
                        }
                    } else {
                        WARN("hw name not found {}\r\n", var->get_o2_comp_hw_name());
                    }

                    final_value = inter_value * o2_coeff;

                    WARN("{} O2 COMP: inter value = {}, final value = {} o2_comp: {} \r\n", var->get_usr_name(), inter_value, final_value, o2_comp);
                }
                break;
            case 2: {
                tag_unit = "\t" + var->get_tag_final_unit();
                inter_value = var->get_inter_value_avg_report();
                double press_comp = var->get_press_comp();
                double temp_comp = var->get_temp_comp();
                double localvalue, temp_coeff = 1, press_coeff = 1;
                if(tag_man_->get_inter_value_avg_report_by_hwname(
                            var->get_press_comp_hw_name(),
                            localvalue)) {
                    if(localvalue != 0 && press_comp != 0)
                        press_coeff = press_comp / localvalue;
                }

                if(tag_man_->get_inter_value_avg_report_by_hwname(
                            var->get_temp_comp_hw_name(),
                            localvalue)) {
                    if(((273 + localvalue) != 0) && ((273 + temp_comp) != 0))
                        temp_coeff = (273 + temp_comp) / (273 + localvalue);
                }

                final_value = inter_value * temp_coeff * press_coeff;

                WARN("{} T-P COMP: inter value = {}, final value = {} t-comp: {} p-comp: {} \r\n", var->get_usr_name(),
                     inter_value, final_value, temp_comp, press_comp);
            }
                break;
            default:
                break;
            }

            if(final_value < 0) final_value = 0;

            tag_value = "\t" + std::to_string(final_value);

            tag_name = var->get_usr_name();
            tag_time = get_current_time_cont();



            vect.push_back(tag_name);
            vect.push_back(tag_value);
            vect.push_back(tag_unit);
            vect.push_back(tag_time);
            vect.push_back(status);



            //LREP("{}\t{}\t{}\r\n", tag_name, tag_value, tag_unit);
            logger_write_row(vect);
        } else {
        }
    }
}



void Logger::logger_create_file(const std::string &name) {
    loader_ = std::make_shared<lib::datalog::CSVFile>(name);
    int err = loader_->open(true);
    if(err != 0) {
        ERR("open file error: {}\n", err);
    }
}

Logger::FileDesc Logger::logger_create_logfile()
{
    FileDesc file;
    std::string year, month, day;
    char temp[30];
    app::user_config cfg = app::userconfig::instance()->get_user_config();
    //logger_close();
    std::string time = get_current_time_cont();    
    std::string filename = cfg.filename.tentinh + "_" +
            cfg.filename.tencoso + "_" +
            cfg.filename.tentram + "_" +
            time +".txt";


    year = lib::convert::to_string<int>(time_now_->tm_year + 1900) + "/";
    sprintf(temp, "%02d/", time_now_->tm_mon + 1);
    temp[3] = 0;
    month = std::string((const char*)temp, 3);
    sprintf(temp, "%02d/", time_now_->tm_mday);
    temp[3] = 0;
    day = std::string((const char*)temp, 3);


    file.remote_file_path = year + month + day;
    file.local_file_path = local_folder_path_ + year + month + day;

    if(!lib::filesystem::create_directories(file.local_file_path)) {
        WARN("create directory {} failed\n", file.local_file_path);
    } else {
        LREP("create folder {} success \r\n", file.local_file_path);
    }

    logger_create_file(file.local_file_path + filename);

    std::vector<std::string> header;
    header.push_back("Thông số");
    header.push_back("Giá trị\t");
    header.push_back("Đơn vị");
    header.push_back("Thời gian\t");
    header.push_back("Trạng thái TB");
    logger_write_row(header);    
    file.file_name = filename;

    LREP("create new file: {}\n", file.local_file_path + filename);

    return file;
}

void Logger::logger_write_row(const std::vector<std::string> row)
{
    if(loader_) {
        if(loader_->is_open()) {
            loader_->write_row(row);
        } else {
            ERR("loader_ is not open\n");
        }
    } else {
        ERR("loader_ is null\n");
    }
}

void Logger::logger_write_row(const std::string &x,
                                 const std::string &y,
                                 const std::string &z)
{

    std::vector<std::string> data;

    data.push_back(get_current_time_cont() + "\t");
    data.push_back(x + "\t\t");
    data.push_back(y + "\t\t");
    data.push_back(z + "\t");

    logger_write_row(data);
}


void Logger::logger_close()
{
    if(loader_ != nullptr) {
        loader_->close();
        loader_.reset();
    }
}

void Logger::thread_function()
{
    int log_min = (config_.server.log_dur) >= 1 ? (config_.server.log_dur) : 1;
    int last_min = 0;
    bool logged = false;
    while(listen_run_) {
        std::this_thread::sleep_for(1_s);
        std::cout << "#"; fflush(stdout);
        read_tag_value();        
        get_current_time();
        if(time_now_->tm_min != last_min && logged) {
            logged = false;
        }

        if(logged == false && (time_now_->tm_min) % log_min == 0) {
            FileDesc file = logger_create_logfile();
            read_save_tag_value();
            mutex_.lock();
            add_buff_point(file);
            mutex_.unlock();
            last_min = time_now_->tm_min;
            logged = true;
        }
    }
}

void Logger::thread_background()
{
    int err;
    while(backgnd_run_) {
        std::this_thread::sleep_for(2_s);
        // send file that sent failed before
        bool got_info = false;
        FileDesc file;
        std::size_t size;

        mutex_.lock();
        size = get_buff_size();
        mutex_.unlock();

        if(size > 0) {
            LREP("wait size: {}\n", size);
            mutex_.lock();
            got_info = get_buff_first(file);
            mutex_.unlock();

            //directory_manager(local_folder_path_, max_num_file_);
            if(got_info) {
                //LREP("try send file: {} \n", file.file_name);
                err = remote_put(file.file_name, file.local_file_path, file.remote_file_path);
                if(err == FTP_ERR_NONE || err == FTP_ERR_FILE) {
                    mutex_.lock();
                    rem_buff_first();
                    mutex_.unlock();
                    LREP("remove file name: {}\n", file.file_name);                    
                } else {
                    //LREP("stor file failed err = {}\n", err);
                }
            } else {
                ERR("can not get file info when list not empty {}", 1);
            }
        }
    }
}

void Logger::directory_manager(const std::string &dir, int maxNumberOfFiles){
    DIR *dp;
    struct dirent *entry, *oldestFile;
    struct stat statbuf;
    int numberOfEntries=0;
    time_t t_oldest;

    std::string curr_dir = pwd();

    time(&t_oldest);
    //printf("now:%s\n", ctime(&t_oldest));
    if((dp = opendir(dir.c_str())) != NULL) {
       chdir(dir.c_str());
       while((entry = readdir(dp)) != NULL) {
          lstat(entry->d_name, &statbuf);
          if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
             continue;
          //printf("%s\t%s", entry->d_name, ctime(&statbuf.st_mtime));
             numberOfEntries++;
          if(difftime(statbuf.st_mtime, t_oldest) < 0){
                t_oldest = statbuf.st_mtime;
             oldestFile = entry;
          }
      }
    } else {
        //printf("folder %s can not open\n", dir.c_str());
        return;
    }

    //printf("\n\n\n%s", oldestFile->d_name);
    if(numberOfEntries >= maxNumberOfFiles)
       remove(oldestFile->d_name);

    //printf("\noldest time:%s", ctime(&t_oldest));
    closedir(dp);

    chdir(curr_dir.c_str());
}



std::string Logger::pwd()
{
    std::string ret = "";
    FILE *file = popen("pwd", "r");
    char re[1000];
    memset(re, 0, 1000);

    while(fgets(re, 1000, file))
    {
        //printf("%s", re);
        ret = std::string(re);
    }
    pclose(file);

    return ret;
}


void Logger::load_buff_file(const std::string &filename)
{
    file_name_ = filename;
    std::ofstream outfile;
    std::ifstream infile;

    if(!lib::filesystem::exists(file_name_)) {
        outfile.open(file_name_, std::ios::out);
        if(outfile.is_open()) {           
            outfile.close();
            /*{   FileDesc file;
                add_buff_point(file);
                rem_buff_first();
            }*/
        } else {
            std::cout << "can't open file: " << file_name_ << std::endl;
        }
    } else {
        infile.open(file_name_, std::ios::in);
        if(infile.is_open()) {
            std::stringstream stream;
            stream << infile.rdbuf();
            Json::Reader reader;
            bool ready =  reader.parse(stream.str(), root_);
            if(!ready){
                ERR("json parse failed\n");
            }
        }
        infile.close();
    }
}

void Logger::add_buff_point(const TcpClientSock::FileDesc &file) {
    Json::Value element;
    element["filename"] = file.file_name;
    element["localpath"] = file.local_file_path;
    element["remotepath"] = file.remote_file_path;
    root_.append(element);
    save_buff_file();
}

void Logger::rem_buff_first() {
    if(root_.size() > 0) {
        Json::Value value = root_[0];
        root_.removeIndex(0, &value);
        save_buff_file();
        std::cout << "root size " << root_.size() << std::endl;
    } else {
        std::cout << "root size " << root_.size() << std::endl;
    }
}

bool Logger::get_buff_first(TcpClientSock::FileDesc &point) {
    if(root_.size() > 0) {
        Json::Value elem = root_[0];
        point.file_name = elem["filename"].asString();
        point.local_file_path = elem["localpath"].asString();
        point.remote_file_path = elem["remotepath"].asString();
        return true;
    }
    return false;
}

void Logger::save_buff_file() {
    Json::StyledStreamWriter writer;
    std::ofstream outfile;
    outfile.open(file_name_, std::ios::out);
    if(outfile.is_open()) {
        writer.write(outfile,root_);
        outfile.close();
        //std::cout << "file saved to: " <<  file_name_ << std::endl;;
    } else {
        std::cout << "can't open file: " << file_name_ << std::endl;
    }
}


}




}
