#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QDebug>
#include <QListWidgetItem>
#include <QFile>
#include <QFileDialog>
#include <QTimer>
#include <QTime>

#define BUFFER_SIZE 8192

MainWindow::MainWindow(QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), socket(socket)
{
    ui->setupUi(this);
    ui->tabWidget->setTabEnabled(2, false);
    timer = new QTimer(this);
    timer->setInterval(1000);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(on_recvButton_clicked()));
    this->on_tabWidget_currentChanged(0);
    this->on_tabWidget_currentChanged(5);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QString req = QString("QUIT");
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    timer->stop();
    if (index == 0) { // Friends Info
        QString req = QString("LS");
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->friendsListWidget->clear();
            QStringList friendsList = anslist[1].trimmed().split('\t');
            for (int i = 0; i < friendsList.length(); i += 2) {
                if (friendsList[i + 1] == "Offline") friendsList[i + 1] = "离线";
                else friendsList[i + 1] = "在线";
                ui->friendsListWidget->addItem(friendsList[i] + " (" + friendsList[i + 1] + ")");
            }
        } else {
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
    else if (index == 1) { // Users Info
        QString req = QString("SEARCH");
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->usersListWidget->clear();
            QStringList usersList = anslist[1].trimmed().split('\t');
            for (int i = 0; i < usersList.length(); i += 2) {
                if (usersList[i + 1] == "Offline") usersList[i + 1] = "离线";
                else usersList[i + 1] = "在线";
                ui->usersListWidget->addItem(usersList[i] + " (" + usersList[i + 1] + ")");
            }
        } else {
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
    else if (index == 2) { // Send Info
        ui->sendInfoLabel->setText("当前聊天对象：" + this->currentChatName);
        timer->start();
    }
    else if (index == 3) { // Recv Info
        timer->start();
    }
    else if (index == 4) { // SendAll Info

    }
    else if (index == 5) { // Profile Info
        QString req = QString("PROFILE");
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->usersListWidget->clear();
            QStringList argList = anslist[1].trimmed().split('\t');
            for (QString arg: argList) {
                ui->profileLabel->setText("用户名：" + argList[0]);
            }
        } else {
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
}

void MainWindow::on_addButton_clicked()
{
    QListWidgetItem* currentItem = ui->usersListWidget->currentItem();
    if (currentItem == NULL) {
        ui->usersInfoLabel->setText("添加失败：请选择用户");
    }
    else {
        QString username = currentItem->text();
        QString req = QString("ADD") + QString("\t") + username.split(' ').first();
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->usersInfoLabel->setText("添加成功");
        } else {
            ui->usersInfoLabel->setText("添加失败：好友已存在");
        }
    }
}

void MainWindow::on_chatButton_clicked()
{
    QListWidgetItem* currentItem = ui->friendsListWidget->currentItem();
    if (currentItem == NULL) {
        ui->friendsInfoLabel->setText("聊天失败：请选择用户");
    }
    else {
        QString username = currentItem->text().split(' ').first();
        QString req = QString("CHAT") + QString("\t") + username;
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->friendsInfoLabel->setText("聊天成功");
            this->currentChatName = username;
            ui->tabWidget->setTabEnabled(2, true);
            ui->sendListWidget->clear();
        } else {
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
}

void MainWindow::on_sendButton_clicked()
{
    timer->stop();
    QString message = ui->sendTextEdit->toPlainText();
    if (message.length() == 0) {
        ui->sendInfoLabel->setText("发送失败：请输入信息");
    }
    QString req = QString("SENDMSG") + QString("\t") + message.replace('\t', '\n');
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        ui->sendInfoLabel->setText("发送成功");
        ui->sendListWidget->addItem(QString("我 (" + QTime::currentTime().toString() + ") :\n") + message);
    } else {
        QMessageBox::information(this, anslist[0], anslist[1]);
    }
    timer->start();
}

void MainWindow::on_recvButton_clicked()
{
    timer->stop();
    QString req = QString("RECVMSG");
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        QStringList messageList = anslist[1].trimmed().split('\t');
        for (int i = 0; i < messageList.size(); i += 2) {
            QString username = messageList[i];
            QString message = messageList[i + 1];
            if (username == this->currentChatName) {
                ui->sendListWidget->addItem(username + " (" + QTime::currentTime().toString() + ") :\n" + message);
            }
            ui->recvListWidget->addItem(username + " (" + QTime::currentTime().toString() + ") :\n" + message);
            ui->recvInfoLabel->setText("接收成功");
        }
    } else {
        ui->recvInfoLabel->setText("无最新消息");
    }
    timer->start();
}

void MainWindow::on_recvFileButton_clicked()
{
    timer->stop();
    QString req = QString("RECVFILE");
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        ui->recvInfoLabel->setText("开始接收");
        QStringList argList = anslist[1].trimmed().split('\t');
        QString username = argList[0];
        int length = argList[1].toInt();

        this->socket->write(req.toStdString().c_str());
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString filename = QString(cans).trimmed();

        QByteArray filearr;
        for (int i = 0; i < length - 2; i++) {
            this->socket->write(req.toStdString().c_str());
            memset(cans, 0, sizeof(cans));
            this->socket->waitForReadyRead();
            int len = this->socket->read(cans, BUFFER_SIZE);
            for (int j = 0; j < len; j++) filearr.append(cans[j]);
        }
        QFile file("/Users/yuhui/Downloads/" + filename);
        file.open(QIODevice::ReadWrite);
        file.write(filearr);
        if (username == this->currentChatName) {
            ui->sendListWidget->addItem(username + " (" + QTime::currentTime().toString() + ") :\n文件：" + filename);
        }
        ui->recvListWidget->addItem(username + " (" + QTime::currentTime().toString() + ") :\n文件：" + filename);
        ui->recvInfoLabel->setText("接收成功");
    } else {
        ui->recvInfoLabel->setText("无最新文件");
    }
    timer->start();
}

void MainWindow::on_sendFileButton_clicked()
{
    timer->stop();
    QString filename = QFileDialog::getOpenFileName(this);
    if (filename.length() == 0) {
        ui->sendInfoLabel->setText("发送失败：请选择文件");
    } else {
        QString req = QString("SENDFILE") + QString("\t") + filename.split('/').last();
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->sendInfoLabel->setText("开始发送");
            QFile file(filename);
            file.open(QIODevice::ReadOnly);
            while (true) {
                QByteArray filearr = file.read(4096);
                if (filearr.length() == 0) break;
                QByteArray reqarr = QByteArray("SENDFILE") + QByteArray("\t") + filearr;
                this->socket->write(reqarr);
                memset(cans, 0, sizeof(cans));
                this->socket->waitForReadyRead();
                this->socket->read(cans, BUFFER_SIZE);
            }
            ui->sendListWidget->addItem(QString("我 (" + QTime::currentTime().toString() + ") :\n文件：") + filename);
            ui->sendInfoLabel->setText("发送成功");
        } else {
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
    timer->start();
}


void MainWindow::on_exitButton_clicked()
{
    QString req = QString("EXIT");
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        ui->sendInfoLabel->setText("退出成功");
        ui->tabWidget->setTabEnabled(2, false);
    } else {
        QMessageBox::information(this, anslist[0], anslist[1]);
    }
}

void MainWindow::on_syncAction_triggered()
{
    this->on_tabWidget_currentChanged(0);
    this->on_tabWidget_currentChanged(1);
    QString req = QString("SYNC");
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        QMessageBox::information(this, "同步成功", "同步成功");
    } else {
        QMessageBox::information(this, anslist[0], anslist[1]);
    }
}


void MainWindow::on_sendAllButton_clicked()
{
    QString message = ui->sendAllTextEdit->toPlainText();
    if (message.length() == 0) {
        ui->sendAllInfoLabel->setText("发送失败：请输入信息");
    }
    QString req = QString("SENDALL") + QString("\t") + message.replace('\t', '\n');
    this->socket->write(req.toStdString().c_str());
    char cans[BUFFER_SIZE];
    memset(cans, 0, sizeof(cans));
    this->socket->waitForReadyRead();
    this->socket->read(cans, BUFFER_SIZE);
    QString ans = QString(cans);
    qDebug() << ans;
    QStringList anslist = ans.split(':');
    if (anslist[0] == "SUCCESS") {
        ui->sendAllInfoLabel->setText("发送成功");
    } else {
        QMessageBox::information(this, anslist[0], anslist[1]);
    }
}
