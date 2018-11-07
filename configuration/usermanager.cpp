#include "usermanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpp_lib/filesystem.h>

#define FILE_PATH_INFO_LC  "/users.db"

UserManager::UserManager()
{

    int rc;
    std::string full_path = lib::filesystem::current_path().string() + FILE_PATH_INFO_LC;
    bool exist = fileExists(full_path);
    if(exist) {
        std::cout << "file existed !\r\n";
        rc = sqlite3_open(full_path.c_str(), &db_);
        if(rc == SQLITE_OK) {
            std::cout << "open connection ok\r\n";
            createDefaultTable();
        } else {
            printf("ERROR opening SQLite DB 'users.db': %s\n", sqlite3_errmsg(db_));
        }
    } else {
        std::cout << "file is not existed !\r\n";
        std::fstream file;
        std::cout << "create new file\r\n";
        file.open(full_path, std::ios::out);
        if(file.is_open()) {
            file.close();
            std::cout << "create file success\r\n";
            rc = sqlite3_open(full_path.c_str(), &db_);
            if(rc == SQLITE_OK) {
                std::cout << "open connection ok\r\n";
                createDefaultTable();
            } else {
                printf("ERROR opening SQLite DB 'users.db': %s\n", sqlite3_errmsg(db_));
            }
        } else {
            std::cout << "can not create file\r\n";
        }
    }    
}

UserManager::~UserManager()
{
    if(db_) {
        sqlite3_close(db_);
        db_ = NULL;
    }
}

bool UserManager::addUser(const std::string &username_, const std::string &password_, const std::string &role_)
{    
    if(checkUserExisted(username_)) {
        std::cout << "user " << username_ << " is existed !" <<  std::endl;;
        return false;
    }

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "INSERT INTO users (username, password, role) VALUES ('%s', '%s', '%s')", username_.c_str(), password_.c_str(), role_.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;

}

bool UserManager::checkUserExisted(const std::string &username_)
{
    if(db_ == NULL)
        return false;

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "select * from users where username='%s';", username_.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc == SQLITE_ROW) {
        return true;
    } else {
        //printf("CHECK RECORD ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }

}

bool UserManager::createTable(const std::string &table)
{
    if(db_ == NULL)
        return false;

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "create table %s (username text primary key, password text, role text);", table.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        //printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool UserManager::checkTableExisted(const std::string &table)
{
    if(db_ == NULL)
        return false;

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "SELECT * FROM %s;", table.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        //printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool UserManager::checkUserPasswdMatch(const std::string &username, const std::string &passwd, std::string &role)
{
    if(!checkUserExisted(username)) {
        std::cout << "user not found !" <<  username << std::endl;
        return false;
    }

    role.clear();
    bool ret = false;
    char *cmd;
    int rc = asprintf(&cmd, "SELECT username, password, role FROM users WHERE username='%s' AND password='%s';",
             username.c_str(), passwd.c_str());

    (void)rc;

    query(std::string(cmd), [&](const std::vector<std::string> &data) -> int {
        if(data.size() >= 3) {
            if(data.at(0) == username && data.at(1) == passwd) {
                ret = true;
            }
            role = data.at(2);
        }
        return BREAK;
    });
    return ret;
}

bool UserManager::changePasswd(const std::string &username_, const std::string passwd_)
{
    if(!checkUserExisted(username_)) {
        std::cout << "user not found !" <<  username_ << std::endl;
        return false;
    }

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "update users set password='%s' where username='%s';", passwd_.c_str(), username_.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool UserManager::resetPasswd(const std::string &username_)
{
    if(!checkUserExisted(username_)) {
        std::cout << "user not is existed !" <<  username_  << std::endl;
        return false;
    }

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "update users set password='123456a@' where username='%s';", username_.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool UserManager::delUser(const std::string &username)
{
    if(!checkUserExisted(username)) {
        std::cout << "user not found !" <<  username << std::endl;
        return false;
    }

    int rc;
    sqlite3_stmt *stmt;
    char *cmd;
    rc = asprintf(&cmd, "delete from users where username='%s';", username.c_str());
    sqlite3_prepare_v2(db_, cmd, strlen(cmd), &stmt, NULL);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    delete cmd;
    if (rc != SQLITE_DONE) {
        printf("ERROR: %s\n", sqlite3_errmsg(db_));
        return false;
    }
    return true;
}

bool UserManager::createDefaultTable()
{    
    if(checkTableExisted("users")) {
        createDefaultRecord();
        return true;
    } else {
        if(createTable("users")) {
            createDefaultRecord();
            return true;
        }
        return false;
    }
}

bool UserManager::createDefaultRecord()
{
    std::cout << "careate default record\r\n";
    if(!checkUserExisted("root")) {
        if(!addUser("Vatcom", "root", "service"))
            std::cout << "add account root failed \n";
    } else {
        std::cout << "account root is existed\n";
    }

    if(!checkUserExisted("admin")) {
        if(!addUser("admin", "admin", "admin"))
            std::cout << "add account admin failed \n";
    } else {
        std::cout << "account admin is existed\n";
    }

    if(!checkUserExisted("user")) {
        if(!addUser("user", "123456a@", "user"))
           std::cout << "add account user failed \n";
    } else {
        std::cout << "account user is existed\r\n";
    }

    fflush(stdout);

    return true;
}

bool UserManager::login(const std::string &username, const std::string &passwd, std::string &role)
{
    return checkUserPasswdMatch(username, passwd, role);
}

bool UserManager::listAllAccount()
{
    bool ret = false;

    query("SELECT * FROM users;", [&](const std::vector<std::string> &data) -> int {
        for(auto &var : data)
            std::cout << var << " ";
        std::cout << std::endl;
        ret = true;
        return CONTINUE;
    });
    return ret;
}

bool UserManager::getListAccount(std::vector<std::string> &vect)
{
    bool ret = false;
    vect.clear();

    query("SELECT * FROM users;", [&](const std::vector<std::string> &data) -> int {
        for(auto &var : data) {
            std::cout << var << " ";
        }
        vect.push_back(data.at(0) + ":" + data.at(2)+ " ");
        std::cout << std::endl;
        ret = true;
        return CONTINUE;
    });
    return ret;
}

UsersLogin::UsersLogin()
{
    handler_ = std::make_shared<UserManager>();
}

UsersLogin::~UsersLogin()
{
    destroy();
}

bool UsersLogin::isLoggedOn(const std::string &username)
{
    for(auto &var : list_logged_on_) {
        if(var == username) {
            return true;
        }
    }
    return false;
}

void UsersLogin::remove_logged_on(const std::string &username)
{
//    std::list<std::string>::iterator iter = list_logged_on_.begin();
//    while(iter != list_logged_on_.end()) {
//        if(*iter == username)     {
//            iter = list_logged_on_.erase(iter);
//        }
//        continue;
//        iter++;
//    }
}
