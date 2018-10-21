#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <sqlite3.h>
#include <sys/stat.h>
#include <string>
#include <system_error>
#include <iostream>
#include <fstream>
#include <cpp_lib/database.h>
#include <cpp_lib/pattern.h>
#include <list>
using namespace lib::database;

class UserManager : public sqlite
{
public:
    UserManager();
    ~UserManager();
    bool addUser(const std::string &username_, const std::string &password_, const std::string &role_);
    bool checkUserExisted(const std::string &username_);
    bool createTable(const std::string &table);
    bool checkTableExisted(const std::string &table);
    bool checkUserPasswdMatch(const std::string &username, const std::string &passwd, std::string &role);
    bool changePasswd(const std::string &username_, const std::string passwd_);
    bool resetPasswd(const std::string &username_);
    bool delUser(const std::string &username);
    bool createDefaultTable();
    bool createDefaultRecord();
    bool login(const std::string &username, const std::string &passwd, std::string &role);
    bool listAllAccount();
    bool getListAccount(std::vector<std::string> &vect);

    bool fileExists(const std::string   &file) {
        struct stat buf;
        return (stat(file.c_str(), &buf) == 0);
    }
};


class UsersLogin : public lib::pattern::singleton<UsersLogin>
{
public:
    UsersLogin();
    ~UsersLogin();

    bool addUser(const std::string &username_, const std::string &password_, const std::string &role_) {
        bool ret;
        locker.lock();
        ret = handler_->addUser(username_, password_, role_);
        locker.unlock();
        return ret;
    }

    bool checkUserPasswdMatch(const std::string &username, const std::string &passwd, std::string &role) {
        bool ret;
        locker.lock();

//        if(!isLoggedOn(username)) {
//            ret =  handler_->checkUserPasswdMatch(username, passwd, role);
//            if(ret == true) {
//                list_logged_on_.push_back(username);
//            } else {
//                role = "badd_info";
//            }
//        } else {
//            ret = false;
//            role = "user_loggedon";
//        }

        ret =  handler_->checkUserPasswdMatch(username, passwd, role);
        if(ret == false) {
            role = "bad_info";
        }

        locker.unlock();
        return ret;
    }

    bool changePasswd(const std::string &username_, const std::string passwd_) {
        bool ret;
        locker.lock();
        ret = handler_->changePasswd(username_, passwd_);
        locker.unlock();
        return ret;
    }

    bool resetPasswd(const std::string &username_) {
        bool ret;
        locker.lock();
        ret = handler_->resetPasswd(username_);
        locker.unlock();
        return ret;
    }

    bool delUser(const std::string &username) {
        bool ret;
        locker.lock();
        ret = handler_->delUser(username);
        locker.unlock();
        return ret;
    }

    bool listAllAccount() {
        bool ret;
        locker.lock();
        ret = handler_->listAllAccount();
        locker.unlock();
        return ret;
    }

    bool getListAccount(std::vector<std::string> &vect) {
        bool ret;
        locker.lock();
        ret = handler_->getListAccount(vect);
        locker.unlock();
        return ret;
    }


    bool isLoggedOn(const std::string &username);
    void remove_logged_on(const std::string &username);

private:
    std::mutex                       locker;
    std::shared_ptr<UserManager>     handler_;
    std::list<std::string>           list_logged_on_;
};

#endif // USERMANAGER_H
