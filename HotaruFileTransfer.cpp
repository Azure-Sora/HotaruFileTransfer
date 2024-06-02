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
    setWindowIcon(QIcon(":/HotaruFileTransfer/assets/icon.png"));

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

    //关于界面
    connect(ui->about, &QMenu::aboutToShow, [=]() {
        auto a = new AboutWindow(this);
        a->show();
        });

    //选中设备才能按连接
    ui->btn_connectTo->setEnabled(false);
    connect(ui->deviceList, &QTableWidget::cellClicked, [=]() {
        ui->btn_connectTo->setEnabled(true);
        });

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
    ui->act_debugMode->trigger();


    /*
    * 
    * 处理广播设备，连接设备部分
    * 
    */

    connect(boardcastTimer, &QTimer::timeout, [=]() {//发送设备发现广播
        //构造设备名与状态
        auto status = (ui->stackedWidget->currentIndex() == 0) ? "ready" : "connected";
        auto data = (ui->deviceNameEdit->text() + "@_@" + status).toUtf8();
        
        boardcast->writeDatagram(data.data(), QHostAddress::Broadcast, 11451);
        });

    connect(deviceTimer, &QTimer::timeout, [=]() {
        deviceTimeout();
        });

    connect(boardcastReceiver, &QUdpSocket::readyRead, [=]() {//接收设备发现广播
        while (boardcastReceiver->hasPendingDatagrams())
        {
            QByteArray data;
            data.resize(boardcastReceiver->pendingDatagramSize());
            QHostAddress host;
            //quint16 port;
            boardcastReceiver->readDatagram(data.data(), data.size(), &host);

            
            if (!isDebugMode() && host.isEqual(NetworkUtil::getValidAddr()))//跳过自己
            {
                return;
            }

            if (!devices.contains(QHostAddress(host.toIPv4Address())))//新设备
            {
                auto infos = QString::fromUtf8(data).split("@_@");
                auto newdevice = ActiveDevice(QHostAddress(host.toIPv4Address()), infos[0], infos[1]);
                devices.append(newdevice);
                refreshTable();
            }
            else//牢设备
            {
                auto infos = QString::fromUtf8(data).split("@_@");
                auto& dvinfo = devices[devices.indexOf(QHostAddress(host.toIPv4Address()))];
                if (dvinfo.status != infos[1])//更新状态
                {
                    dvinfo.status = QString(infos[1]);
                    refreshTable();
                }
                dvinfo.lifeTime = 10;
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
        if (selectedDevice[2]->text() == "已连接到其他设备")
        {
            QMessageBox::information(this, "提示", "该设备已与其他设备连接");
            return;
        }
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
        auto dirPth = QFileDialog::getExistingDirectory(this, "选择文件夹");
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
            return;
        }
        else
        {
            auto filesList = rawStr.split("|");
            sendFiles(filesList);
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
                    fileSize = 0;
                    bytesCompleted = 0;
                    updateProgressBar();
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

/**
* @brief 用超时来处理设备离线
*/
void HotaruFileTransfer::deviceTimeout()
{
    for (int i = 0; i < devices.size(); ++i)
    {
        devices[i].lifeTime -= 1;
        if (devices[i].lifeTime <= 0)
        {
            devices.removeAt(i);
            --i;//移除i处的元素后i指到了新的元素上，需要回退一位
            refreshTable();
        }
    }
}

/**
* @brief 刷新设备列表
*/
void HotaruFileTransfer::refreshTable()//
{
    ui->deviceList->clearContents();
    ui->deviceList->setRowCount(0);
    ui->btn_connectTo->setDisabled(true);
    for (int i = 0; i < devices.size(); i++)
    {
        ui->deviceList->insertRow(i);
        ui->deviceList->setItem(i, 0, new QTableWidgetItem(devices[i].IPAddr.toString()));
        ui->deviceList->setItem(i, 1, new QTableWidgetItem(devices[i].deviceName));
        QString statusStr;
        if (devices[i].status == "ready")
        {
            statusStr = "可连接";
        }
        if (devices[i].status == "connected")
        {
            statusStr = "已连接到其他设备";
        }
        ui->deviceList->setItem(i, 2, new QTableWidgetItem(statusStr));
    }
}

/**
* @brief 更新进度条
*/
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

/**
* @brief 发送完成后启用按钮
*/
void HotaruFileTransfer::finishSendingFile()
{
    ui->btn_serverSendFile->setEnabled(true);
    ui->btn_serverDisconnect->setEnabled(true);
    ui->filePathEdit->setEnabled(true);
    ui->btn_serverChooseDir->setEnabled(true);;
    ui->btn_serverChooseFile->setEnabled(true);;
}

/**
* @brief 实际发送文件的代码
* @param file：文件在本机的绝对路径，用于打开文件
* @param fileName：文件发送后的相对路径，用于保存文件
* @return 发送是否成功
*/
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
    
    qfile.close();

    fileSize = 0;
    bytesCompleted = 0;

    updateProgressBar();

    return true;
}

/**
* @brief 处理发送多个文件，最终交给sendSingleFile发送
*/
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

/**
* @brief 递归找出文件夹中所有文件
* @param dirPath：本次处理的文件夹
* @return 所有文件
*/
QFileInfoList HotaruFileTransfer::getDirFiles(QString dirPath)
{
    QDir dir(dirPath);
    auto fileList = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoSymLinks);//列出文件
    auto folderList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);//列出子文件夹

    for (auto& folder : folderList)//递归所有子文件夹
    {
        auto childFileList = getDirFiles(folder.absoluteFilePath());
        fileList.append(childFileList);
    }

    return fileList;
}

/**
* @brief 处理发送文件夹，最终交给sendSingleFile发送
*/
void HotaruFileTransfer::sendDirectory(QString dir)
{
    dir.replace('\\','/');//把路径分隔符统一格式
    auto files = getDirFiles(dir);
    int counter = 0;//发送文件计数
    for (auto& file : files)
    {
        auto relativeName = file.absoluteFilePath().replace(dir, dir.split("/").last());//把文件绝对路径中属于发送的文件夹的父目录的部分替换成文件夹名
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

