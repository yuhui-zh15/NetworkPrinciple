#ifndef DATA_CPP
#define DATA_CPP

#include "User.h"
#include "Utils.h"
#include <vector>
#include <string>
#include <fstream>

class Data
{
public:
    Data() {
        this->usercnt = 0;
        LoadData();
    }
    
    void AddUser(std::string name, std::string password) {
        User user(usercnt, name, password);
        this->users.push_back(user);
        this->usercnt++;
    }

    std::vector<User> GetUsers() { return this->users; }
    int GetUserCount() { return this->usercnt; }
    User GetUser(int index) { return this->users[index]; }

    int IsUserExisted(std::string name) {
        bool flag = false;
        int i;
        for (i = 0; i < users.size(); i++) {
            if (users[i].GetName() == name) { flag = true; break; }
        }
        return flag? i: -1;
    }

    void SetUser(User user, int index) { this->users[index] = user; }

    void LoadData() {
        std::ifstream fin("data");
        if (fin.is_open()) {
            std::string line;
            getline(fin, line);
            std::vector<std::string> splitline = Split(line, '\t');
            for (int i = 0; i < splitline.size(); i++) {
                int id = std::stoi(splitline[i]);
                User user(id);
                user.LoadUser();
                users.push_back(user);
                usercnt++;
            }
            fin.close();
        }
    }

    void StoreData() {
        std::ofstream fout("data");
        if (fout.is_open()) {
            for (int i = 0; i < users.size(); i++) {
                fout << i << '\t';
                users[i].StoreUser();
            }
            fout.close();
        }
    }

private:
    std::vector<User> users;
    int usercnt;
};

#endif