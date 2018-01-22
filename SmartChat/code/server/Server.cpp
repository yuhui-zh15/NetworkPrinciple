#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <vector>
#include "Data.h"
#include "Utils.h"
using namespace std;

#define MAX_CLIENTS 100

static unsigned int cli_count = 0;
static int tid = 0;

Data database;

/* Client structure */
struct Client {
    struct sockaddr_in addr; /* Client remote address */
    int connfd; /* Connection file descriptor */
    int tid; /* Thread id */
    int uid; /* Client unique identifier */
};

// pthread_mutex_t mutex;

Client *clients[MAX_CLIENTS];

/* Add client to queue */
void QueueAdd(Client *cl) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = cl;
            return;
        }
    }
}

/* Delete client from queue */
void QueueDelete(int tid) {
    for(int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->tid == tid) {
                clients[i] = NULL;
                return;
            }
        }
    }
}

/* Send message to sender */
void SendMessageSelf(const char *s, int connfd, int length) {
    write(connfd, s, length);
}

/* Print ip address */
void PrintClientAddr(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xFF, (addr.sin_addr.s_addr & 0xFF00)>>8, (addr.sin_addr.s_addr & 0xFF0000)>>16, (addr.sin_addr.s_addr & 0xFF000000)>>24);
}

std::string HandleLogin(std::string name, std::string password, Client* client) {
    std::string ret;
    int index = database.IsUserExisted(name);
    if (index == -1) {
        ret = "FAIL: No User";
    } 
    else {
        User user = database.GetUser(index);
        if (user.GetPassword() == password) {
            ret = "SUCCESS";
            user.SetStatus(1);
            database.SetUser(user, index);
        }
        else {
            ret = "FAIL: Wrong Password";
        }
    }
    client->uid = index;
    return ret;
}

std::string HandleRegister(std::string name, std::string password) {
    std::string ret;
    int index = database.IsUserExisted(name);
    if (index == -1) {
        database.AddUser(name, password);
        ret = "SUCCESS";
    }
    else {
        ret = "FAIL: User Exist";
    }
    return ret;
}

std::string HandleSearch() {
    std::string ret;
    std::vector<User> users = database.GetUsers();
    if (users.size() > 0) {
        ret += "SUCCESS: ";
        for (int i = 0; i < users.size(); i++) {
            ret += users[i].GetName();
            ret += '\t';
            ret += users[i].GetStatus();
            ret += '\t';
        }
    }
    else {
        ret += "FAIL: No User Exist";
    }
    return ret;
}

std::string HandleLs(int uid) {
    std::string ret;
    User user = database.GetUser(uid);
    std::vector<int> usersidx = user.GetFriends();
    if (usersidx.size() > 0) {
        ret += "SUCCESS: ";
        for (int i = 0; i < usersidx.size(); i++) {
            ret += database.GetUser(usersidx[i]).GetName();
            ret += '\t';
            ret += database.GetUser(usersidx[i]).GetStatus();
            ret += '\t';
        }
    }
    else {
        ret += "FAIL: No Friend";
    }
    return ret;
}

std::string HandleAdd(int uid, std::string name) {
    std::string ret;
    User user = database.GetUser(uid);
    int index = database.IsUserExisted(name);
    if (index == -1) {
        ret += "FAIL: User Not Exist";
    }
    else {
        int friendindex = user.IsFrientExisted(index);
        if (friendindex == -1) {
            user.AddFriend(index);
            User frienduser = database.GetUser(index);
            if (frienduser.IsFrientExisted(uid) == -1) {
                frienduser.AddFriend(uid);
            }
            database.SetUser(frienduser, index);
            database.SetUser(user, uid);
            ret += "SUCCESS";
        }
        else {
            ret += "FAIL: Friend Exist";
        }
    }

    return ret;
}

std::string HandleProfile(int uid) {
    std::string ret;
    User user = database.GetUser(uid);
    ret += "SUCCESS: ";
    ret += user.GetName();
    ret += '\t';
    ret += user.GetPassword();
    return ret;
}

std::string HandleRecvmsg(int uid) {
    std::string ret;
    User user = database.GetUser(uid);
    std::vector<std::string> messages = user.GetMessages();
    if (messages.size() == 0) {
        ret += "FAIL: No Recent Message";
    }
    else {
        ret += "SUCCESS: ";
        for (int i = 0; i < messages.size(); i++) {
            ret += messages[i];
            ret += '\t';
        }
    }
    user.CleanMessages();
    database.SetUser(user, uid);
    return ret;
}

int recvfd = 0;

std::string HandleRecvfile(int uid) {
    std::string ret;
    User user = database.GetUser(uid);
    std::vector<std::string> file = user.GetFile();
    if (file.size() == 0) {
        ret += "FAIL: No Recent File";
    }
    else {
        if (recvfd == 0) {
            ret += "SUCCESS: ";
            ret += file[recvfd];
            ret += '\t';
            ret += std::to_string(file.size());
        } else {
            ret += file[recvfd];
        }
        recvfd++;
        if (recvfd == file.size()) {
            recvfd = 0;
            user.CleanFile();
        }
    }
    database.SetUser(user, uid);
    return ret;
}

std::string HandleChat(int uid, std::string name) {
    std::string ret;
    User user = database.GetUser(uid);
    int index = database.IsUserExisted(name);
    if (index == -1) {
        ret += "FAIL: User Not Exist";
    }
    else {
        user.SetChatId(index);
        ret += "SUCCESS";
    }
    database.SetUser(user, uid);
    return ret;
}

std::string HandleSendmsg(int uid, std::string message) {
    std::string ret;
    User user = database.GetUser(uid);
    int chatid = user.GetChatId();
    if (chatid == -1) {
        ret += "FAIL: No Chat ID";
    }
    else {
        User chatuser = database.GetUser(chatid);
        chatuser.AddMessage(user.GetName(), message);
        database.SetUser(chatuser, chatid);
        ret += "SUCCESS";
    }
    return ret;
}

std::string HandleSendfile(int uid, std::string file) {
    std::string ret;
    User user = database.GetUser(uid);
    int chatid = user.GetChatId();
    if (chatid == -1) {
        ret += "FAIL: No Chat ID";
    }
    else {
        User chatuser = database.GetUser(chatid);
        chatuser.AddFile(user.GetName(), file);
        database.SetUser(chatuser, chatid);
        ret += "SUCCESS";
    }
    return ret;
}

std::string HandleExit(int uid) {
    std::string ret;
    User user = database.GetUser(uid);
    int chatid = user.GetChatId();
    if (chatid == -1) {
        ret += "FAIL: No Chat ID";
    }
    else {
        user.SetChatId(-1);
        ret += "SUCCESS";
    }
    database.SetUser(user, uid);
    return ret;
}

std::string HandleSync() {
    std::string ret;
    // pthread_mutex_lock(&mutex);        
    database.StoreData();
    // pthread_mutex_unlock(&mutex);
    ret += "SUCCESS";
    return ret;
}

void HandleQuit(int uid) {
    User user = database.GetUser(uid);
    user.SetStatus(0);
    database.SetUser(user, uid);
}

std::string HandleSendAll(int uid, std::string message) {
    std::string ret;
    User user = database.GetUser(uid);
    std::vector<User> users = database.GetUsers();
    for (int i = 0; i < users.size(); i++) {
        User chatuser = database.GetUser(i);
        chatuser.AddMessage(user.GetName(), message);
        database.SetUser(chatuser, i);
    }
    ret += "SUCCESS";
    return ret;
}

/* Handle all communication with the client */
void *HandleClient(void *arg) {
    char buff_in[8192];
    int rlen;

    cli_count++;
    Client *cli = (Client *)arg;

    printf("Accept ");
    PrintClientAddr(cli->addr);
    printf(" referenced by %d\n", cli->tid);

    /* Receive input from client */
    while ((rlen = read(cli->connfd, buff_in, sizeof(buff_in) - 1)) > 0)
    {
        buff_in[rlen] = 0;
        std::string str_in;
        str_in.resize(rlen);
        for (int i = 0; i < rlen; i++) str_in[i] = buff_in[i];

        std::cerr << "LOG: received \'" << buff_in
                  << "\' from " << cli->tid << std::endl;

        std::vector<std::string> str_in_list = Split(str_in, '\t');
        std::string command = str_in_list[0];
        std::string msg;
        if (command == "LOGIN") {
            std::string username = str_in_list[1];
            std::string password = str_in_list[2];
            msg = HandleLogin(username, password, cli);
        }
        else if (command == "REGISTER") {
            std::string username = str_in_list[1];
            std::string password = str_in_list[2];
            msg = HandleRegister(username, password);
        }
        else if (command == "SEARCH") {
            msg = HandleSearch();
        }
        else if (command == "LS") {
            msg = HandleLs(cli->uid);
        }
        else if (command == "ADD") {
            std::string username = str_in_list[1];
            msg = HandleAdd(cli->uid, username);
        }
        else if (command == "PROFILE") {
            msg = HandleProfile(cli->uid);
        }
        else if (command == "CHAT") {
            std::string username = str_in_list[1];
            msg = HandleChat(cli->uid, username);
        }
        else if (command == "RECVMSG") {
            msg = HandleRecvmsg(cli->uid);
        }
        else if (command == "RECVFILE") {
            msg = HandleRecvfile(cli->uid);
        }
        else if (command == "SENDMSG") {
            std::string message = str_in.substr(command.length() + 1, str_in.length() - command.length() - 1);
            msg = HandleSendmsg(cli->uid, message);
        }
        else if (command == "SENDFILE") {
            std::string file = str_in.substr(command.length() + 1, str_in.length() - command.length() - 1);
            msg = HandleSendfile(cli->uid, file);
        }
        else if (command == "SYNC") {
            msg = HandleSync();
        }
        else if (command == "EXIT") {
            msg = HandleExit(cli->uid);
        }
        else if (command == "QUIT") {
            HandleQuit(cli->uid);
            break;
        }
        else if (command == "SENDALL") {
            std::string message = str_in.substr(command.length() + 1, str_in.length() - command.length() - 1);
            msg = HandleSendAll(cli->uid, message);
        }
        else {
            msg = "FAIL: Unknown Command";
        }
        SendMessageSelf(msg.c_str(), cli->connfd, msg.length());
    }

    /* Close connection */
    close(cli->connfd);

    /* Delete client from queue and yeild thread */
    QueueDelete(cli->tid);
    printf("Leave ");
    PrintClientAddr(cli->addr);
    printf(" referenced by %d\n", cli->tid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t thread;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    /* Bind */
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Socket binding failed");
        return 1;
    }

    /* Listen */
    if (listen(listenfd, 10) < 0) {
        perror("Socket listening failed");
        return 1;
    }

    std::cerr << "<SERVER STARTED>" << std::endl;

    /* Accept clients */
    while (true) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        /* Check if max clients is reached */
        if ((cli_count + 1) == MAX_CLIENTS) {
            std::cerr << "<<MAX CLIENTS REACHED\n";
            std::cerr << "<<REJECT ";
            PrintClientAddr(cli_addr);
            std::cerr << "\n";
            close(connfd);
            continue;
        }

        /* Client settings */
        Client *cli = new Client;
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->tid = tid++;
        cli->uid = -1;

        /* Add client to the queue and fork thread */
        QueueAdd(cli);
        pthread_create(&thread, NULL, &HandleClient, (void*)cli);
    }
}
