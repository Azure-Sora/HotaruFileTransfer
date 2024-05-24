#include "HotaruFileTransfer.h"

/*
* UDP port:
* 11451:广播
* 11452:建立连接
* TCP port:
* 11452:连接传输
*/


HotaruFileTransfer::HotaruFileTransfer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HotaruFileTransferClass())
{
    ui->setupUi(this);

    /*
    * 
    * 软件初始化部分
    * 
    */
    ui->stackedWidget->setCurrentIndex(0);

    //设备表格
    ui->deviceList->setColumnWidth(0, 300);
    ui->deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->deviceList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->deviceList->setSelectionMode(QAbstractItemView::SingleSelection);

    //计时器和套接字
    deviceTimer = new QTimer;
    boardcastTimer = new QTimer;

    boardcastReceiver = new QUdpSocket(this);
    boardcast = new QUdpSocket(this);
    connectHelper = new QUdpSocket(this);

    server = new QTcpServer(this);
    socket = new QTcpSocket(this);

    outStream = new QDataStream(socket);
    inStream = new QDataStream(socket);

    //发送方界面
    ui->filePathEdit->setPlaceholderText("请输入文件或文件夹路径...");


    //server->listen(QHostAddress::Any, 11452);

    //connect(ui->btn_initService, &QPushButton::clicked, [=]() {//初始化广播
    //    ui->btn_initService->setDisabled(true);
    //    boardcastTimer->start(250);
    //    deviceTimer->start(100);
    //    boardcastReceiver->bind(QHostAddress::Any, 11451, QUdpSocket::ShareAddress);
    //    });

    connect(ui->btn_recv, &QPushButton::clicked, [=]() {//初始化接收广播
        ui->btn_recv->setDisabled(true);
        //
        server->listen(QHostAddress::Any, 11452);
        //
        deviceTimer->start(100);
        boardcastReceiver->bind(QHostAddress::Any, 11451, QUdpSocket::ShareAddress);
        });

    connect(ui->btn_start, &QPushButton::clicked, [=]() {//初始化发送广播
        boardcastTimer->start(250);
        //qDebug() << NetworkUtil::getValidAddr();
        boardcast->bind(NetworkUtil::getValidAddr(), 11451, QAbstractSocket::ShareAddress);
        connectHelper->bind(QHostAddress::Any, 11452, QUdpSocket::ShareAddress);
        ui->btn_start->setDisabled(true);
        });

    /*
    * 
    * 调试用内容
    * 
    */

    connect(ui->page_main, &QAction::triggered, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
        });
    connect(ui->page_server, &QAction::triggered, [=]() {
        ui->stackedWidget->setCurrentIndex(1);
        });
    connect(ui->page_client, &QAction::triggered, [=]() {
        ui->stackedWidget->setCurrentIndex(2);
        });



    /*
    * 
    * 处理广播设备，连接设备部分
    * 
    */

    connect(boardcastTimer, &QTimer::timeout, [=]() {//发送广播
        //auto time = QTime::currentTime();
        auto data = QByteArray("ready");
        //auto data = QByteArray(time.toString().toLocal8Bit());
        
        boardcast->writeDatagram(data.data(), QHostAddress::Broadcast, 11451);
        });

    connect(deviceTimer, &QTimer::timeout, [=]() {
        deviceTimeout();
        //refreshTable();
        });

    connect(boardcastReceiver, &QUdpSocket::readyRead, [=]() {//接收广播
        while (boardcastReceiver->hasPendingDatagrams())
        {
            QByteArray data;
            data.resize(boardcastReceiver->pendingDatagramSize());
            QHostAddress host;
            //quint16 port;
            boardcastReceiver->readDatagram(data.data(), data.size(), &host);

            //ui->deviceList->
            //!deviceExists(QHostAddress(host.toIPv4Address()))
            
            
            if (!devices.contains(QHostAddress(host.toIPv4Address())))
            {
                auto newdevice = ActiveDevice(QHostAddress(host.toIPv4Address()), QString(data));
                devices.append(newdevice);
                refreshTable();
            }
            else
            {
                devices[devices.indexOf(QHostAddress(host.toIPv4Address()))].lifeTime = 10;
            }
        }

        });

    connect(connectHelper, &QUdpSocket::readyRead, [=]() {
        QByteArray data;
        data.resize(connectHelper->pendingDatagramSize());
        QHostAddress host;
        //quint16 port;
        connectHelper->readDatagram(data.data(), data.size(), &host);
        if (QString(data).startsWith("connect"))
        {

            QMessageBox connectMessage(QMessageBox::Information, "有新的连接", QHostAddress(host.toIPv4Address()).toString() + "想要给您发送文件，将在5秒后自动拒绝", QMessageBox::Yes | QMessageBox::No, this);
            QTimer::singleShot(5000, &connectMessage, &QMessageBox::close);
            int ret = QMessageBox::No;
            ret = connectMessage.exec();
            if (ret == QMessageBox::Yes)
            {
                socket->connectToHost(host, 11452);
                if (socket->waitForConnected(3000))
                {
                    ui->stackedWidget->setCurrentIndex(2);
                    inStream->setDevice(socket);
                    ui->serverLog->append(createLog("已连接到" + QHostAddress(host.toIPv4Address()).toString() + "\n"));
                    return;
                }
            }
            else
            {
                return;
            }


        }
        });

    connect(ui->btn_connectTo, &QPushButton::clicked, [=]() {//连接到另一台设备
        auto selectedDevice = ui->deviceList->selectedItems();
        auto ip = QHostAddress(selectedDevice[0]->text());
        auto data = QByteArray("connect");
        boardcast->writeDatagram(data, ip, 11452);

        });

    connect(server, &QTcpServer::newConnection, [=]() {//接收设备连接
        socket = server->nextPendingConnection();
        outStream->setDevice(socket);
        ui->stackedWidget->setCurrentIndex(1);
        });

    /*
    * 
    * 处理发送方功能
    * 
    */

    connect(ui->btn_serverChooseFile, &QPushButton::clicked, [=]() {
        auto files = QFileDialog::getOpenFileNames(this, "选择一个或多个文件", QString(), "所有文件(*.*)");
        ui->filePathEdit->setText(files.join("|"));
        });
    connect(ui->btn_serverChooseDir, &QPushButton::clicked, [=]() {
        QString dirPth = QFileDialog::getExistingDirectory(this, "选择文件夹");
        ui->filePathEdit->setText(dirPth);
        });

    connect(ui->btn_serverDisconnect, &QPushButton::clicked, [=]() {
        socket->disconnectFromHost();
        QMessageBox::information(this, "提示", "将在文件传输完成后断开连接");
        ui->stackedWidget->setCurrentIndex(0);
        });
    connect(socket, &QTcpSocket::disconnected, [=]() {
        QMessageBox::information(this, "提示", "已断开连接");
        ui->stackedWidget->setCurrentIndex(0);
        });

    connect(ui->btn_serverSendFile, &QPushButton::clicked, [=]() {
        ui->btn_serverSendFile->setDisabled(true);
        ui->btn_serverDisconnect->setDisabled(true);
        ui->filePathEdit->setDisabled(true);
        ui->btn_serverChooseDir->setDisabled(true);
        ui->btn_serverChooseFile->setDisabled(true);

        auto rawStr = ui->filePathEdit->toPlainText();
        QFileInfo fileInfo(rawStr);
        if (fileInfo.isDir())
        {
            std::thread sender(&HotaruFileTransfer::sendDirectory, this, rawStr);
            sender.detach();

            //ui->serverLog->append(createLog("文件夹" + rawStr));
            return;
        }
        else
        {
            auto filesList = rawStr.split("|");
            /*std::thread sender(&HotaruFileTransfer::sendFiles, this, filesList);
            sender.detach();*/
            sendFiles(filesList);

            /*for (auto& fl : filesList)
            {
                ui->serverLog->append(createLog("文件" + fl));
            }*/
            return;
        }
        
        });

    /*
    * 
    * 处理接收方功能
    * 
    */
    

    connect(socket, &QTcpSocket::readyRead, [=]() {
        if (fileSize == 0)
        {
            QString fileName;
            (*inStream) >> fileSize >> fileName;
            operatingFile.setFileName(fileName);
            operatingFile.open(QIODevice::WriteOnly);
            return;
        }
        else
        {
            auto size = qMin(socket->bytesAvailable(), fileSize - bytesCompleted);
            /*if (size == 0)
            {
                operatingFile.close();
                return;
            }*/

            QByteArray data(size, 0);
            inStream->readRawData(data.data(), size);
            operatingFile.write(data);
            bytesCompleted += size;

            if (fileSize == bytesCompleted)
            {
                ui->clientLog->append(createLog("已成功接收" + operatingFile.fileName()));
                fileSize = 0;
                bytesCompleted = 0;
                operatingFile.close();
            }

        }
        });

}

HotaruFileTransfer::~HotaruFileTransfer()
{
    delete ui;
}

QString HotaruFileTransfer::createLog(QString str)
{
    auto time = QDateTime::currentDateTime();
    auto timeStr = time.toString("yyyy-MM-dd hh:mm:ss");
    return "[" + timeStr + "] " + str;
}

void HotaruFileTransfer::deviceTimeout()
{
    for (int i = 0; i < devices.size(); ++i)
    {
        devices[i].lifeTime -= 1;
        if (devices[i].lifeTime <= 0)
        {
            devices.removeAt(i);
            --i;
            refreshTable();
        }
    }
}

void HotaruFileTransfer::refreshTable()
{
    ui->deviceList->clearContents();
    ui->deviceList->setRowCount(0);
    for (int i = 0; i < devices.size(); i++)
    {
        ui->deviceList->insertRow(i);
        ui->deviceList->setItem(i, 0, new QTableWidgetItem(devices[i].IPAddr.toString()));
        ui->deviceList->setItem(i, 1, new QTableWidgetItem(devices[i].status));
    }
}

void HotaruFileTransfer::finishSendingFile()
{
    ui->btn_serverSendFile->setEnabled(true);
    ui->btn_serverDisconnect->setEnabled(true);
    ui->filePathEdit->setEnabled(true);
    ui->btn_serverChooseDir->setEnabled(true);;
    ui->btn_serverChooseFile->setEnabled(true);;
}

bool HotaruFileTransfer::sendSingleFile(QString file, QString fileName)
{
    static constexpr int fragSize = 1024;//以1024B作文件块大小

    QFile qfile(file);
    if (!qfile.open(QIODevice::ReadOnly))
    {
        ui->serverLog->append(createLog("打开文件" + fileName + "失败！\n"));
        return false;
    }
    fileSize = qfile.size();
    int sentSize = 0;

    
    (*outStream) << fileSize << fileName;
    socket->waitForBytesWritten();

    while (!qfile.atEnd())
    {
        auto data = qfile.read(fragSize);
        outStream->writeRawData(data.constData(), data.size());
        bytesCompleted += data.size();
        if (bytesCompleted % 10240 == 0) QApplication::processEvents();
        socket->waitForBytesWritten();
    }
    
    qfile.close();

    fileSize = 0;
    bytesCompleted = 0;

    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

void HotaruFileTransfer::sendFiles(QStringList files)
{
    if (files.length() == 0)
    {
        ui->serverLog->append(createLog("文件路径为空！\n"));
        finishSendingFile();
        return;
    }
    for (auto& file : files)
    {
        if (!QFileInfo(file).isFile())
        {
            ui->serverLog->append(createLog("输入了错误的文件名！\n"));
            finishSendingFile();
            return;
        }
    }
    for (int i = 0; i < files.length(); ++i)
    {
        if (sendSingleFile(files[i], QFileInfo(files[i]).fileName()))
        {
            ui->serverLog->append(createLog("文件已发送[" + QString::number(i + 1) + "/" + QString::number(files.length()) + "]\n"));
        }
        else
        {
            ui->serverLog->append(createLog("文件发送失败！"));
        }
    }
    //finishSendingFile();
    return;
}

void HotaruFileTransfer::sendDirectory(QString dir)
{
    return;
}

