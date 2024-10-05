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
    m_BytesWrites=0;   //已发送数据
    m_BytesToWrites=0;  //剩下数据
    m_LoadSizes=4*1024;    //每次发送大小

    m_TcpMsgClient=new QTcpSocket(this);     //处理聊天消息的socket
    //m_TcpFileClient=new QTcpSocket(this);    //处理文件传输消息的socket

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

    connect(m_TcpMsgClient,&QTcpSocket::connected,this,&MainDialog::OnConnectedFunc);
    connect(m_TcpMsgClient,&QTcpSocket::readyRead,this,&MainDialog::OnSocketReadyReadFunc);
    connect(m_TcpMsgClient,&QTcpSocket::disconnected,this,&MainDialog::OnDisConnectedFunc);

    ui->plainTextEdit_SendMsg->setFocus();  //将光标定位到发送消息框

    //安装事件过滤器
    ui->plainTextEdit_SendMsg->installEventFilter(this);

    //系统托盘函数
    MySystemTrayFunc();

    //传递文件传输错误
    connect(m_TcpMsgClient,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),this,&MainDialog::DisplayErrorFunc);

}

MainDialog::~MainDialog()
{
    delete ui;
}

void MainDialog::paintEvent(QPaintEvent *event)
{
//    QPainter *ptr=new QPainter(this);
//    QPixmap pix;
//    pix.load(":/Image/imrxshluzrn.webp");
    //    ptr->drawPixmap(0,0,width(),height(),pix);
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
    if(m_TcpMsgClient->state() == QAbstractSocket::ConnectedState){
        m_TcpMsgClient->disconnectFromHost();
    }
//    if(m_TcpFileClient->state() == QAbstractSocket::ConnectedState){
//        m_TcpFileClient->disconnectFromHost();
//    }

    event->accept();

}

void MainDialog::OnConnectedFunc()  //客户端连接服务器
{
    //connected()连接的槽函数
    ui->plainTextEdit->appendPlainText("[--------客户端与服务器连接成功--------]\n");
    ui->plainTextEdit->appendPlainText("[IP Address: "+m_TcpMsgClient->peerAddress().toString()+"]"
                                       +"[Port: "+QString::number(m_TcpMsgClient->peerPort())+"]");

    ui->pushButton_Connect->setEnabled(false);
    ui->pushButton_Disconnect->setEnabled(true);
}

void MainDialog::OnDisConnectedFunc()   //客户端与服务器断开
{
    //disConnected()的槽函数
    ui->plainTextEdit->appendPlainText("[--------客户端断与服务器连接--------]\n");
    m_TcpMsgClient->close();
    m_TcpMsgClient->deleteLater();

    ui->pushButton_Connect->setEnabled(true);
    ui->pushButton_Disconnect->setEnabled(false);
}

//接收消息
void MainDialog::OnSocketReadyReadFunc()    //读取服务器socket传输数据信息
{
    //日期时间
    QDateTime CurrentDateTime=QDateTime::currentDateTime();
    QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");
    //while(m_TcpMsgClient->canReadLine()){
        ui->plainTextEdit->appendPlainText("[服务器消息 "+datetimes+"]："+m_TcpMsgClient->readAll());
    //}
}

//实现文件传输，及进度条
void MainDialog::UpdateClientProgressFunc(qint64 numBytes)
{
    //确认已发送文件大小
    m_BytesWrites+=(int)numBytes;

    //进度条更新
    ui->progressBar->setMaximum(m_TotalBytes);
    ui->progressBar->setValue(m_BytesWrites);

    if(m_BytesToWrites>0){//剩下数据大小
        //从文件中取出数据到发送缓冲区  ，每次4kb
        m_OutDataBlock=m_Localfile->read(qMin(m_BytesToWrites,m_LoadSizes));
        //发送缓冲区数据，并计算剩下数据大小
        m_BytesToWrites=m_BytesToWrites-(qint64)m_TcpMsgClient->write(m_OutDataBlock);

        //清空缓冲区
        m_OutDataBlock.resize(0);
    }
    else{
        m_Localfile->close();

    }

    if(m_BytesWrites==m_TotalBytes){
        //日期时间
        QDateTime CurrentDateTime=QDateTime::currentDateTime();
        QString datetimes=CurrentDateTime.toString("yyyy/MM/dd HH:mm:ss");
        ui->plainTextEdit->appendPlainText(QString("[----文件：%1 已成功传输到服务器---- %2]").arg(m_FileNames).arg(datetimes));

        // 重置状态
        ui->pushButton_SendMsg->setEnabled(true);
        disconnect(m_TcpMsgClient,&QTcpSocket::bytesWritten,this,&MainDialog::UpdateClientProgressFunc);
        m_TotalBytes = 0;
        m_BytesWrites = 0;
        m_OutDataBlock.resize(0);
        ui->progressBar->reset(); // 重置进度条
        m_Localfile->close();

    }
}

//异常处理
void MainDialog::DisplayErrorFunc(QAbstractSocket::SocketError)
{
    m_TcpMsgClient->close();
    ui->progressBar->close();

    ui->plainTextEdit->appendPlainText(QString("文件传输发生错误"));

}

//系统托盘
void MainDialog::MySystemTrayFunc()
{
    QIcon qIcons(":/Image/setting.png");
    MysystemTrays=new QSystemTrayIcon(this);
    MysystemTrays->setIcon(qIcons);

    //提示消息
    MysystemTrays->setToolTip("tcp客户端");

    qConnectAction=new QAction("连接服务器",this);
    connect(qConnectAction,&QAction::triggered,this,&MainDialog::on_pushButton_Connect_clicked);

    qdisConnectAction=new QAction("断开服务器",this);
    connect(qdisConnectAction,&QAction::triggered,this,&MainDialog::on_pushButton_Disconnect_clicked);

    qAboutAction=new QAction("关于",this);
    connect(qAboutAction,&QAction::triggered,this,[&](){
        QMessageBox::about(this, "About", "客户端\n\n"
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

//连接服务器按钮
void MainDialog::on_pushButton_Connect_clicked()
{
    //获取ip和端口
     QString ipaddress=ui->comboBox_ServerIP->currentText();
     quint64 port=ui->spinBox_ServerPort->value();

     //连接服务器处理操作
     m_TcpMsgClient->connectToHost(ipaddress,port);
     //m_TcpFileClient->connectToHost(ipaddress,port);
}

//断开服务器
void MainDialog::on_pushButton_Disconnect_clicked()
{
    if(m_TcpMsgClient->state() == QAbstractSocket::ConnectedState){
        m_TcpMsgClient->disconnectFromHost();
    }
//    if(m_TcpFileClient->state() == QAbstractSocket::ConnectedState){
//        m_TcpFileClient->disconnectFromHost();
//    }
}

void MainDialog::on_pushButton_Exit_clicked()
{
    //退出系统则关闭Socket
    m_TcpMsgClient->close();
    //m_TcpFileClient->close();
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
    //避免冲突
    //disconnect(m_TcpMsgClient,&QTcpSocket::bytesWritten,this,&MainDialog::UpdateClientProgressFunc);
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
    ui->plainTextEdit->appendPlainText("[客户端消息 "+datetimes+"]："+strMsg);
    ui->plainTextEdit_SendMsg->clear();
    ui->plainTextEdit_SendMsg->setFocus();  //将光标定位到发送消息框

    QByteArray bytearray= "MSG:" + strMsg.toUtf8();// 添加标识符
    //bytearray.append("\n");

    //发送
    m_TcpMsgClient->write(bytearray);

}

//选择文件
void MainDialog::on_pushButton_SelectFile_clicked()
{
    m_FileNames=QFileDialog::getOpenFileName(this,"请选择要传输的文件","", "All Files (*.*)");
    if(!m_FileNames.isEmpty()){
        ui->plainTextEdit->appendPlainText(QString("[客户端打开文件为：%1 成功] ").arg(m_FileNames));
        ui->pushButton_SendFile->setEnabled(true);//启用发送按钮
    }
}

//发送文件
void MainDialog::on_pushButton_SendFile_clicked()
{
    m_Localfile=new QFile(m_FileNames);
    if(!m_Localfile->open(QFile::ReadOnly)){
        QMessageBox::critical(this,"错误","打开文件错误,请重新检查",QMessageBox::Yes);
        return;
    }

    m_TotalBytes=m_Localfile->size();//发送文件的大小

    QDataStream sendDataOut(&m_OutDataBlock,QIODevice::WriteOnly);
    //求文件名
    QString strCurrentFileName=m_FileNames.right(m_FileNames.size()-m_FileNames.lastIndexOf('/')-1);

    //依次写入文件总大小信息空间，文件名大小信息空间，文件名
    //两个占位符
    sendDataOut<<qint64(0)<<qint64(0)<<strCurrentFileName;


    m_TotalBytes=m_TotalBytes+m_OutDataBlock.size();

    // 移动到开头
    sendDataOut.device()->seek(0);

    //返回sendDataOut的开始，用实际大小替代占位符
    sendDataOut<<m_TotalBytes<<qint64(strCurrentFileName.size());

    //QByteArray fileData = "FILE:" + m_OutDataBlock; // 添加标识符

    QByteArray fileData =  m_OutDataBlock;

    connect(m_TcpMsgClient,&QTcpSocket::bytesWritten,this,&MainDialog::UpdateClientProgressFunc);

    //发送文件期间禁用发送消息按钮
    ui->pushButton_SendMsg->setEnabled(false);
    //发送完前面数据之后，剩下的数据大小
    m_BytesToWrites=m_TotalBytes - m_TcpMsgClient->write(fileData);

    m_FileNames="";
    m_OutDataBlock.resize(0);

}
