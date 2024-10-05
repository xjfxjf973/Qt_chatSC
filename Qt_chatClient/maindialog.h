#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QFile>
#include <QFileDialog>

#include <QHostAddress>
#include <QHostInfo>

#include <QMessageBox>
#include <QLabel>
#include <QDateTime>

#include <QPainter>
#include <QPixmap>

#include <QDateTime>

//系统相关头文件
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>


QT_BEGIN_NAMESPACE
namespace Ui { class MainDialog; }
QT_END_NAMESPACE

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = nullptr);
    ~MainDialog();

    void paintEvent(QPaintEvent* event);
private:
    QTcpSocket* m_TcpMsgClient;     //处理聊天消息的socket
    QTcpSocket* m_TcpFileClient;    //处理文件传输消息的socket

    QFile* m_Localfile;     //需要传输的文件
    qint64 m_TotalBytes;    //数据总大小
    qint64 m_BytesWrites;   //已发送数据
    qint64 m_BytesToWrites;  //剩下数据
    qint64 m_LoadSizes;     //每次发送
    QString m_FileNames;    //保存文件消息

    QByteArray m_OutDataBlock;  //数据缓冲区

    //重写eventFilter
    bool eventFilter(QObject *obj,QEvent *event) override;

    QString GetLocalIPAddress();    //获取本机IP地址

    void closeEvent(QCloseEvent* event) override;    //重写关闭事件

    //系统托盘
    QSystemTrayIcon *MysystemTrays;

    QMenu *pContextMenu;    //托盘菜单

    QAction *qConnectAction;
    QAction *qdisConnectAction;
    QAction *qAboutAction;
    QAction *qExitAction;


private slots:
    void OnConnectedFunc();         //客户端连接服务器
    void OnDisConnectedFunc();      //客户端断开服务器
    void OnSocketReadyReadFunc();   //读取服务器socket传输数据信息


    void UpdateClientProgressFunc(qint64 numBytes);  //文件传输及进度条更新
    void DisplayErrorFunc(QAbstractSocket::SocketError);    //异常处理

    void MySystemTrayFunc();       //系统托盘

    //单击按钮事件
    void on_pushButton_Connect_clicked();

    void on_pushButton_Disconnect_clicked();

    void on_pushButton_Exit_clicked();

    void on_pushButton_ClearMsg_clicked();

    void on_pushButton_SendMsg_clicked();

    void on_pushButton_SelectFile_clicked();

    void on_pushButton_SendFile_clicked();

private:
    Ui::MainDialog *ui;
};
#endif // MAINDIALOG_H
