#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTcpSocket *socket, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_tabWidget_currentChanged(int index);
    void on_addButton_clicked();
    void on_chatButton_clicked();
    void on_sendButton_clicked();
    void on_recvButton_clicked();
    void on_exitButton_clicked();
    void on_syncAction_triggered();
    void on_sendFileButton_clicked();
    void on_recvFileButton_clicked();
    void on_sendAllButton_clicked();

private:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QString currentChatName;
    QTimer *timer;

};

#endif // MAINWINDOW_H
