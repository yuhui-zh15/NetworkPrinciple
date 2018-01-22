#ifndef LoginDialog_H
#define LoginDialog_H

#include <QDialog>
#include <QTcpSocket>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();

private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();

private:
    Ui::LoginDialog *ui;
    QTcpSocket *socket;
};

#endif // LoginDialog_H
