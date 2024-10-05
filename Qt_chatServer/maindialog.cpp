#include "maindialog.h"
#include "ui_maindialog.h"

#include <QDebug>

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);

    this->setFixedSize(width(),height());

    m_TotalBytes=0;    //数据总大小
    bytesReceived = 0; // 已接收字节数

//    m_BytesToWrites=0;  //剩下数据
//    m_LoadSizes=4*1024;    //每次发送大小

    m_TcpMsgServer=new QTcpServer(this);     //服务器Server
    m_TcpMsgSocket=new QTcpSocket(this);    //处理消息的socket

    //获取本机IP地址
    QString strLocalIP=GetLocalIPAddress();
    QString strIP=("本机IP地址："+strLocalIP);
    ui->label_LocalIP->setText(strIP);
//    QMessageBox::information(this,"提示",strLocalIP,QMessageBox::Yes);
    //添加到IP选项框中
    ui->comboBox_ServerIP->addItem(strLocalIP);

    //禁用按钮
//    ui->pushButton_Disconnect->setEnabled(false);
//    ui->pushButton_SendFile->setEnabled(false);

    ui->plainTextEdit_SendMsg->setFocus();  //将光标定位到发送消息框

    //安装事件过滤器
    ui->plainTextEdit_SendMsg->installEventFilter(this);

    //系统托盘函数
    MySystemTrayFunc();

    //传递文件传输错误
    //connect(m_TcpFileServer,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&MainDialog::DisplayErrorFunc);

}

MainDialog::~MainDialog()
{
    delete ui;
}

//过滤回车键事件改为发送消息
bool MainDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->plainTextEdit_SendMsg && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            emit ui->pushButton_SendMsg->clicked();  // 发送消息
            return true; // 事件处理完成
        }
    }
    return QWidget::eventFilter(obj, event); // 继续处理其他事件
}

QString MainDialog::GetLocalIPAddress()//获取本机IP地址
{
    //获取本机名称
    QString strHostName = QHostInfo::localHostName();
    qDebug()<<strHostName;
    //通过主机名称获取IP地址
    QHostInfo HostInfo=QHostInfo::fromName(strHostName);

    QString strLocalIP="";

    QList<QHostAddress> addressList=HostInfo.addresses();
    if(!addressList.isEmpty()){
        for (int i =0;i<addressList.count();i++) {
            QHostAddress sHost=addressList.at(i);

            //判断本机IP协议是否为IPv4
            if(QAbstractSocket::IPv4Protocol == sHost.protocol()){
                strLocalIP=sHost.toString();
                break;
            }
        }
    }

    return strLocalIP;
}

void MainDialog::closeEvent(QCloseEvent *event)//重写关闭事件
{
    if(m_TcpMsgSocket->state() == QAbstractSocket::ConnectedState){
        m_TcpMsgSocket->disconnectFromHost();
    }
//    if(m_TcpMsgSocket->state() == QAbstractSocket::ConnectedState){
//        m_TcpMsgSocket->disconnectFromHost();
//    }

    event->accept();

}

void MainDialog::OnListenFunc()
{
    if (m_TcpMsgServer->hasPendingConnections())  //查询是否有新连接
    {
        m_TcpMsgSocket = m_TcpMsgServer->nextPendingConnection(); //获取与真实客户端相连的客户端套接字

        QDateTime CurrentDateTime=QDateTime::currentDateTime();
        QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");
        ui->plainTextEdit->appendPlainText("[--------有客户端连接成功连接成功--------"+datetimes+"]\n");//若有新连接，则提示
        ui->plainTextEdit->appendPlainText("[IP Address: "+m_TcpMsgSocket->peerAddress().toString()+"]"
                                           +"[Port: "+QString::number(m_TcpMsgSocket->peerPort())+"]");

        ui->pushButton_Listen->setEnabled(false);
        ui->pushButton_Dislisten->setEnabled(true);

        connect(m_TcpMsgSocket,&QTcpSocket::readyRead,this,&MainDialog::OnSocketReadyReadFunc); //连接客户端的套接字的有新消息信号到接收消息的槽
        connect(m_TcpMsgSocket,&QTcpSocket::disconnected,this,&MainDialog::OnDislistenFunc); //连接客户端的套接字取消连接信号到取消连接槽
    }
}

void MainDialog::OnDislistenFunc()
{
    if (m_TcpMsgSocket != nullptr)
    {
        ui->plainTextEdit->appendPlainText("[--------断开连接--------]\n");
        m_TcpMsgSocket->close(); //关闭客户端socket
        m_TcpMsgSocket->deleteLater();

        ui->pushButton_Listen->setEnabled(true);
        ui->pushButton_Dislisten->setEnabled(false);
    }
}

//接收消息
void MainDialog::OnSocketReadyReadFunc()
{
    if (m_TcpMsgSocket != nullptr) //与客户端连接的socket，不是nullptr，则说明有客户端存在
    {
        QHostAddress clientaddr = m_TcpMsgSocket->peerAddress(); //获得IP
        int port = m_TcpMsgSocket->peerPort();   //获得端口号

        QByteArray receivedData = m_TcpMsgSocket->readAll();

        // 判断数据类型
        if (receivedData.startsWith("MSG:")) {
            //日期时间
            QDateTime CurrentDateTime=QDateTime::currentDateTime();
            QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");
            QString message = QString::fromUtf8(receivedData.mid(4)); // 取出消息内容
            ui->plainTextEdit->appendPlainText(tr("[") + clientaddr.toString() + tr(" : ") \
                                             + QString::number(port) + tr("  ")+datetimes+"]："+message);
        } else /*if (receivedData.startsWith("FILE:"))*/ {
            // 处理文件数据
            //QByteArray fileData = receivedData.mid(5); // 取出文件数据
            handleClientData(receivedData , m_TcpMsgSocket);
            // 处理文件接收逻辑（例如写入文件等）
        }
//        else {
//            qDebug() << "未知数据类型:" << receivedData;
//        }

    }
}

void MainDialog::MySystemTrayFunc()
{
    QIcon qIcons(":/Image/setting.png");
    MysystemTrays=new QSystemTrayIcon(this);
    MysystemTrays->setIcon(qIcons);

    //提示消息
    MysystemTrays->setToolTip("tcp服务器");

    qConnectAction=new QAction("启动监听",this);
    connect(qConnectAction,&QAction::triggered,this,&MainDialog::on_pushButton_Listen_clicked);

    qdisConnectAction=new QAction("停止监听",this);
    connect(qdisConnectAction,&QAction::triggered,this,&MainDialog::on_pushButton_Dislisten_clicked);

    qAboutAction=new QAction("关于",this);
    connect(qAboutAction,&QAction::triggered,this,[&](){
        QMessageBox::about(this, "About", "服务器\n\n"
            "作 者：必须小心谨慎");
        });

    qExitAction=new QAction("退出",this);
    connect(qExitAction,&QAction::triggered,this,&QApplication::quit);

    //单击图标显示隐藏窗口
    connect(MysystemTrays, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        switch (reason)
        {
        case QSystemTrayIcon::Trigger:
            //显示隐藏界面
            if (isHidden())
                this->show();
            else
                this->hide();
            break;

        default:
            break;
        }
    });

    pContextMenu=new QMenu(this);
    pContextMenu->addAction(qConnectAction);
    pContextMenu->addAction(qdisConnectAction);
    pContextMenu->addAction(qAboutAction);
    pContextMenu->addAction(qExitAction);
    MysystemTrays->setContextMenu(pContextMenu);

    //显示
    MysystemTrays->show();
}

//启动监听
void MainDialog::on_pushButton_Listen_clicked()
{
    ui->plainTextEdit->appendPlainText("[--------服务器开始监听--------]\n");

    //获取ip和端口
     QString ipaddress=ui->comboBox_ServerIP->currentText();
     quint64 port=ui->spinBox_ServerPort->value();


    //调用listen函数监听同时绑定IP和端口号
    if (m_TcpMsgServer->listen(QHostAddress::LocalHost, port)) //判断listen是否成功，成功则继续执行，连接新接收信号槽
    {
        this->connect(m_TcpMsgServer, &QTcpServer::newConnection, this, &MainDialog::OnListenFunc);  //将服务器的新连接信号连接到接收新连接的槽

    }
    else
    {
        QMessageBox::critical(this, "错误", "IP绑定错误，请关闭其它服务端或更改绑定端口号");
    }

    ui->pushButton_Listen->setEnabled(false);
    ui->pushButton_Dislisten->setEnabled(true);
}

//停止监听
void MainDialog::on_pushButton_Dislisten_clicked()
{
    m_TcpMsgServer->close();
    ui->plainTextEdit->appendPlainText("[--------服务器停止监听--------]\n");
    ui->pushButton_Listen->setEnabled(true);
    ui->pushButton_Dislisten->setEnabled(false);
}

//退出系统
void MainDialog::on_pushButton_Exit_clicked()
{
    //退出系统则关闭Socket
    m_TcpMsgSocket->close();
    m_TcpMsgServer->close();
    //m_TcpFileSocket->close();
    this->close();
}

//清除消息
void MainDialog::on_pushButton_ClearMsg_clicked()
{
    ui->plainTextEdit->clear();
}

//发送消息
void MainDialog::on_pushButton_SendMsg_clicked()
{
    if(m_TcpMsgSocket != nullptr){
    //日期时间
    QDateTime CurrentDateTime=QDateTime::currentDateTime();
    QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");

    //
    QString strMsg=ui->plainTextEdit_SendMsg->toPlainText();
//    QMessageBox::information(this,"提示",strMsg,QMessageBox::Yes);

    //判断消息是否为空
    if(strMsg.isEmpty()){
        QMessageBox::critical(this,"错误","发送消息不能为空",QMessageBox::Yes);
        ui->plainTextEdit_SendMsg->setFocus();  //将光标定位到发送消息框
        return;
    }
    //将用户发送的数据消息显示到消息列表中
    ui->plainTextEdit->appendPlainText("[服务器 "+datetimes+"]："+strMsg);
    ui->plainTextEdit_SendMsg->clear();
    ui->plainTextEdit_SendMsg->setFocus();  //将光标定位到发送消息框

    QByteArray bytearray=strMsg.toUtf8();
    //bytearray.append("\n");

    //发送
    m_TcpMsgSocket->write(bytearray);

    }
}

//
void MainDialog::onNewConnection()
{

//    connect(m_TcpMsgSocket, &QTcpSocket::readyRead, this, [=]() {
//        handleClientData(m_TcpMsgSocket);
//    });
//    connect(m_TcpMsgSocket, &QTcpSocket::disconnected, m_TcpMsgSocket, &QTcpSocket::deleteLater);
}

//接收文件
void MainDialog::handleClientData(const QByteArray &fileData, QTcpSocket *m_TcpMsgSocket)
{
    QDataStream in(fileData);

    if (bytesReceived == 0) {

        QDateTime CurrentDateTime=QDateTime::currentDateTime();
        QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");
        ui->plainTextEdit->appendPlainText("[客户端："+m_TcpMsgSocket->peerAddress().toString()+" "
                                           +QString::number(m_TcpMsgSocket->peerPort())+"] 正在上传文件到服务器---"+datetimes);

        // 读取文件总大小和文件名
        in >> m_TotalBytes; // 读取文件总大小
        qint64 fileNameSize;
        in >> fileNameSize; // 读取文件名大小

        if (fileData.size() < fileNameSize + sizeof(qint64) * 2) {
            return; // 文件名未完整接收
        }

        in >> m_FileNames; // 读取文件名

        // 创建本地文件
        localFile = new QFile(m_FileNames);
        if (!localFile->open(QFile::WriteOnly)) {
            qDebug() << "无法打开文件" << m_FileNames;
            return;
        }

        // 初始化进度条
        ui->progressBar->setMaximum(m_TotalBytes);
        bytesReceived = 0; // 重置已接收字节数
    }
    else{
        //第一次传输的文件信息不写入
        localFile->write(fileData); // 写入文件
    }


    // 写入文件数据
    bytesReceived += fileData.size(); // 更新已接收字节数

    // 更新进度条
    ui->progressBar->setValue(bytesReceived);

    // 检查是否接收完所有数据
    if (bytesReceived >= m_TotalBytes) {
        localFile->close();
        localFile->deleteLater();
        qDebug() << "文件接收完毕:" << m_FileNames;

        // 输出接收完成时间
        QDateTime currentDateTime = QDateTime::currentDateTime();
        QString dateTimeString = currentDateTime.toString("yyyy/MM/dd HH:mm:ss");

        ui->plainTextEdit->appendPlainText(QString("[----文件：%1 传输完成---- %2]").arg(m_FileNames).arg(dateTimeString));

        // 重置状态
        m_TotalBytes = 0;
        bytesReceived = 0;
        ui->progressBar->reset(); // 重置进度条
    }
}

