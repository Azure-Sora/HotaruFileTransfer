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
    ui->deviceList->setColumnWidth(0, 150);
    ui->deviceList->setColumnWidth(1, 150);
    ui->deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->deviceList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->deviceList->setSelectionMode(QAbstractItemView::SingleSelection);

    //计时器和套接字
    deviceTimer = new QTimer(this);
    boardcastTimer = new QTimer(this);

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

    connect(ui->btn_initService, &QPushButton::clicked, [=]() {//初始化广播
        ui->btn_initService->setDisabled(true);
        ui->deviceNameEdit->setDisabled(true);

        server->listen(QHostAddress::Any, 11452);
        deviceTimer->start(100);
        boardcastReceiver->bind(QHostAddress::Any, 11451, QUdpSocket::ShareAddress);
        
        boardcast->bind(NetworkUtil::getValidAddr(), 11451, QAbstractSocket::ShareAddress);
        connectHelper->bind(QHostAddress::Any, 11452, QUdpSocket::ShareAddress);
        boardcastTimer->start(250);
        
        });

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
    connect(ui->act_debugMode, &QAction::triggered, [=](bool checked) {
        if (checked)
        {
            ui->page_main->setVisible(true);
            ui->page_server->setVisible(true);
            ui->page_client->setVisible(true);
            ui->btn_start->setVisible(true);
            ui->btn_recv->setVisible(true);
        }
        else
        {
            ui->page_main->setVisible(false);
            ui->page_server->setVisible(false);
            ui->page_client->setVisible(false);
            ui->btn_start->setVisible(false);
            ui->btn_recv->setVisible(false);
        }
        });



    /*
    * 
    * 处理广播设备，连接设备部分
    * 
    */

    connect(boardcastTimer, &QTimer::timeout, [=]() {//发送设备发现广播
        //auto time = QTime::currentTime();
        auto data = (ui->deviceNameEdit->text() + "@_@ready").toLocal8Bit();
        //auto data = QByteArray(time.toString().toLocal8Bit());
        
        boardcast->writeDatagram(data.data(), QHostAddress::Broadcast, 11451);
        });

    connect(deviceTimer, &QTimer::timeout, [=]() {
        deviceTimeout();
        //refreshTable();
        });

    connect(boardcastReceiver, &QUdpSocket::readyRead, [=]() {//接收设备发现广播
        while (boardcastReceiver->hasPendingDatagrams())
        {
            QByteArray data;
            data.resize(boardcastReceiver->pendingDatagramSize());
            QHostAddress host;
            //quint16 port;
            boardcastReceiver->readDatagram(data.data(), data.size(), &host);

            //ui->deviceList->
            //!deviceExists(QHostAddress(host.toIPv4Address()))
            if (!isDebugMode() && host.isEqual(NetworkUtil::getValidAddr()))
            {
                return;
            }
            
            if (!devices.contains(QHostAddress(host.toIPv4Address())))
            {
                auto infos = QString(data).split("@_@");
                auto newdevice = ActiveDevice(QHostAddress(host.toIPv4Address()), infos[0], infos[1]);
                devices.append(newdevice);
                refreshTable();
            }
            else
            {
                devices[devices.indexOf(QHostAddress(host.toIPv4Address()))].lifeTime = 10;
            }
        }

        });

    connect(connectHelper, &QUdpSocket::readyRead, [=]() {//接收设备连接请求
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
                    updateProgressBar();
                    inStream->setDevice(socket);
                    ui->clientLog->append(createLog("已连接到" + QHostAddress(host.toIPv4Address()).toString()));
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
        ui->serverLog->append(createLog("已连接到" + QHostAddress(socket->peerAddress().toIPv4Address()).toString()));
        updateProgressBar();
        });


    /*
    * 
    * 处理发送方功能
    * 
    */

    connect(ui->btn_serverChooseFile, &QPushButton::clicked, [=]() {//选择文件按钮
        auto files = QFileDialog::getOpenFileNames(this, "选择一个或多个文件", QString(), "所有文件(*.*)");
        ui->filePathEdit->setText(files.join("|"));
        });
    connect(ui->btn_serverChooseDir, &QPushButton::clicked, [=]() {//选择文件夹按钮
        QString dirPth = QFileDialog::getExistingDirectory(this, "选择文件夹");
        ui->filePathEdit->setText(dirPth);
        });

    connect(ui->btn_serverDisconnect, &QPushButton::clicked, [=]() {//断开连接相关
        socket->disconnectFromHost();
        QMessageBox::information(this, "提示", "即将断开连接");
        ui->stackedWidget->setCurrentIndex(0);
        });
    connect(socket, &QTcpSocket::disconnected, [=]() {
        QMessageBox::information(this, "提示", "已断开连接");
        ui->stackedWidget->setCurrentIndex(0);
        });

    connect(ui->btn_serverSendFile, &QPushButton::clicked, [=]() {//发送文件
        ui->btn_serverSendFile->setDisabled(true);
        ui->btn_serverDisconnect->setDisabled(true);
        ui->filePathEdit->setDisabled(true);
        ui->btn_serverChooseDir->setDisabled(true);
        ui->btn_serverChooseFile->setDisabled(true);

        auto rawStr = ui->filePathEdit->toPlainText();
        QFileInfo fileInfo(rawStr);//判断是文件还是文件夹
        if (fileInfo.isDir())
        {
            sendDirectory(rawStr);

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

    connect(socket, &QTcpSocket::readyRead, [=]() {//接收文件
        while (socket->bytesAvailable())
        {
            if (fileSize == 0)//不在接收某一文件中
            {
                QString fileName;
                (*inStream) >> fileSize >> fileName;
                operatingFile.setFileName(fileName);
                if (fileName.contains('/'))//存在目录结构，进行处理，创建不存在的目录
                {
                    auto dirName = QFileInfo(operatingFile).absolutePath();
                    QDir dir(dirName);
                    if (!dir.exists())
                    {
                        dir.mkpath(dirName);
                    }
                }
                
                operatingFile.open(QIODevice::WriteOnly);
                ui->clientProgressBar->setFormat("正在接收 " + fileName + " %p%");
                continue;
            }
            else
            {
                auto size = qMin(socket->bytesAvailable(), fileSize - bytesCompleted);//只接收最大不超过这个文件剩下量的数据
                QByteArray data(size, 0);
                inStream->readRawData(data.data(), size);
                operatingFile.write(data);
                bytesCompleted += size;
                if (bytesCompleted % 102400 == 0) updateProgressBar();

                if (fileSize == bytesCompleted)//完成接收一个文件
                {
                    ui->clientLog->append(createLog("已成功接收 " + operatingFile.fileName()));
                    updateProgressBar();
                    fileSize = 0;
                    bytesCompleted = 0;
                    operatingFile.close();
                }

            }
        }
        });

    ui->btn_clientDisconnect->setVisible(false);//这个功能有bug，断开后服务端不知道断开了，修不好
    connect(ui->btn_clientDisconnect, &QPushButton::clicked, [=]() {
        socket->disconnectFromHost();
        QMessageBox::information(this, "提示", "即将断开连接");
        ui->stackedWidget->setCurrentIndex(0);
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
        ui->deviceList->setItem(i, 1, new QTableWidgetItem(devices[i].deviceName));
        ui->deviceList->setItem(i, 2, new QTableWidgetItem(devices[i].status));
    }
}

void HotaruFileTransfer::updateProgressBar()
{
    if (ui->stackedWidget->currentIndex() == 1)//server
    {
        if (fileSize == 0)
        {
            ui->serverProgressBar->setValue(0);
            ui->serverProgressBar->setFormat("");
            return;
        }
        ui->serverProgressBar->setValue(static_cast<int>((static_cast<double>(bytesCompleted) / static_cast<double>(fileSize)) * 100));
        //ui->serverLog->append(QString::number((bytesCompleted / fileSize) * 100.0));
    }
    if (ui->stackedWidget->currentIndex() == 2)//client
    {
        if (fileSize == 0)
        {
            ui->clientProgressBar->setValue(0);
            ui->clientProgressBar->setFormat("");
            return;
        }
        ui->clientProgressBar->setValue(static_cast<int>((static_cast<double>(bytesCompleted) / static_cast<double>(fileSize)) * 100));
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
        ui->serverLog->append(createLog("打开文件" + fileName + "失败！"));
        return false;
    }
    fileSize = qfile.size();
    int sentSize = 0;

    
    (*outStream) << fileSize << fileName;
    ui->serverProgressBar->setFormat("正在发送 " + fileName + " %p%");
    socket->waitForBytesWritten();

    while (!qfile.atEnd())
    {
        auto data = qfile.read(fragSize);
        outStream->writeRawData(data.constData(), data.size());
        bytesCompleted += data.size();
        if (bytesCompleted % 102400 == 0)
        {
            QApplication::processEvents();
            updateProgressBar();
        }
        socket->waitForBytesWritten();
    }
    updateProgressBar();

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
        ui->serverLog->append(createLog("文件路径为空！"));
        finishSendingFile();
        return;
    }
    for (auto& file : files)
    {
        if (!QFileInfo(file).isFile())
        {
            ui->serverLog->append(createLog("输入了错误的文件名！"));
            finishSendingFile();
            return;
        }
    }
    for (int i = 0; i < files.length(); ++i)
    {
        if (sendSingleFile(files[i], QFileInfo(files[i]).fileName()))
        {
            ui->serverLog->append(createLog("文件已发送[" + QString::number(i + 1) + "/" + QString::number(files.length()) + "]"));
        }
        else
        {
            ui->serverLog->append(createLog("文件发送失败！"));
        }
    }
    finishSendingFile();
    return;
}

QFileInfoList HotaruFileTransfer::getDirFiles(QString dirPath)
{
    QDir dir(dirPath);
    auto fileList = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    auto folderList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (auto& folder : folderList)
    {
        auto childFileList = getDirFiles(folder.absoluteFilePath());
        fileList.append(childFileList);
    }

    return fileList;
}

void HotaruFileTransfer::sendDirectory(QString dir)
{
    dir.replace('\\','/');
    auto files = getDirFiles(dir);
    int counter = 0;
    for (auto& file : files)
    {
        auto relativeName = file.absoluteFilePath().replace(dir, dir.split("/").last());
        //ui->serverLog->append(file.absoluteFilePath() + "   " + relativeName);
        
        if (sendSingleFile(file.absoluteFilePath(), relativeName))
        {
            ++counter;
            ui->serverLog->append(createLog("文件已发送[" + QString::number(counter) + "/" + QString::number(files.length()) + "]"));
        }
        else
        {
            ui->serverLog->append(createLog("文件发送失败！"));
        }
    }

    finishSendingFile();
    return;
}

bool HotaruFileTransfer::isDebugMode()
{
    return ui->act_debugMode->isChecked();
}

