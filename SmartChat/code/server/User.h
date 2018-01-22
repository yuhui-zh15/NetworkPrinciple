#ifndef USER_CPP
#define USER_CPP

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "Utils.h"

class User {
public:
    User(int id, std::string name, std::string password) { this->id = id; this->name = name; this->password = password; this->chatid = -1; this->fd = 0; this->status = "Offline"; }
    User(int id) { this->id = id; this->chatid = -1; this->fd = 0; this->status = "Offline"; }
    void SetName(std::string name) { this->name = name; }
    void SetPassword(std::string password) { this->password = password; }
    void SetChatId(int chatid) { this->chatid = chatid; }
    void SetStatus(int status) { this->status = (status == 0)? "Offline": "Online"; }
    void AddFriend(int id) { this->friends.push_back(id); }
    void AddMessage(std::string name, std::string message) { this->messages.push_back(name + '\t' + message); }
    void AddFile(std::string name, std::string message) {
        if (file.size() == 0) {
            file.push_back(name);
        }
        file.push_back(message);
    }

    std::string GetName() { return this->name; }
    std::string GetPassword() { return this->password; }
    int GetChatId() { return this->chatid; }
    std::vector<int> GetFriends() { return this->friends; }
    std::vector<std::string> GetMessages() { return this->messages; }
    std::vector<std::string> GetFile() { return this->file; }
    std::string GetStatus() { return this->status; }
    
    void CleanMessages() { this->messages.clear(); }
    void CleanFile() { this->file.clear(); }

    int IsFrientExisted(int id) {
        bool flag = false;
        int i;
        for (i = 0; i < this->friends.size(); i++) {
            if (this->friends[i] == id) { flag = true; break; }
        }
        return flag? i: -1;
    }

    void StoreUser() {
        std::ofstream fout(std::to_string(this->id));
        if (fout.is_open()) {
            fout << this->name << '\n';
            fout << this->password << '\n';
            fout << this->id << '\n';
            for (int i = 0; i < friends.size(); i++) {
                fout << friends[i] << '\t';
            }
            fout << '\n';
            for (int i = 0; i < messages.size(); i++) {
                fout << messages[i] << '\t';
            }
            fout << '\n';
            fout.close();
        }
    }

    void LoadUser() {
        std::ifstream fin(std::to_string(this->id));
        if (fin.is_open()) {
            std::string line;
            std::vector<std::string> splitline;
            
            getline(fin, this->name);
            getline(fin, this->password);
            getline(fin, line);
            this->id = std::stol(line);
            
            getline(fin, line);
            splitline = Split(line, '\t');
            for (int i = 0; i < splitline.size(); i++) {
                this->friends.push_back(std::stoi(splitline[i]));
            }

            getline(fin, line);
            splitline = Split(line, '\t');
            for (int i = 0; i < splitline.size(); i += 2) {
                this->messages.push_back(splitline[i] + '\t' + splitline[1]);
            }

            fin.close();
        }
    }
    
private:
    std::string name;
    std::string password;
    int chatid;
    std::string status;
    int id;
    std::vector<int> friends;
    std::vector<std::string> messages;
    std::vector<std::string> file;
    int fd;
};

#endif