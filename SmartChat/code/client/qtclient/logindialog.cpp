#include "logindialog.h"
#include "ui_logindialog.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QHostAddress>
#include <string>

#define BUFFER_SIZE 8192

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    this->socket = new QTcpSocket;
    this->socket->connectToHost(QHostAddress("127.0.0.1"), 5000);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}


void LoginDialog::on_loginButton_clicked()
{
    QString name = ui->nameEdit->text();
    QString password = ui->passwordEdit->text();
    if (name.length() == 0 || password.length() == 0) {
        ui->infoLabel->setText("账号密码不能为空");
    }
    else {
        QString req = QString("LOGIN") + QString("\t") + name + QString("\t") + password;
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            MainWindow *w = new MainWindow(this->socket);
            this->close();
            w->show();
        } else {
            ui->infoLabel->setText("登录失败");
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }

}

void LoginDialog::on_registerButton_clicked()
{
    QString name = ui->nameEdit->text();
    QString password = ui->passwordEdit->text();
    if (name.length() == 0 || password.length() == 0) {
        ui->infoLabel->setText("账号密码不能为空");
    }
    else {
        QString req = QString("REGISTER") + QString("\t") + name + QString("\t") + password;
        this->socket->write(req.toStdString().c_str());
        char cans[BUFFER_SIZE];
        memset(cans, 0, sizeof(cans));
        this->socket->waitForReadyRead();
        this->socket->read(cans, BUFFER_SIZE);
        QString ans = QString(cans);
        QStringList anslist = ans.split(':');
        if (anslist[0] == "SUCCESS") {
            ui->infoLabel->setText("注册成功");
        } else {
            ui->infoLabel->setText("注册失败");
            QMessageBox::information(this, anslist[0], anslist[1]);
        }
    }
}
