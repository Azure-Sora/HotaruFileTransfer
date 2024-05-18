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
    //ui->deviceList->setRowCount(1);
    //ui->deviceList->insertRow(0);
    ui->deviceList->setColumnWidth(0, 300);
    ui->deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->deviceList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->deviceList->setSelectionMode(QAbstractItemView::SingleSelection);
    //ui->deviceList->setItem(0, 0, new QTableWidgetItem("114.514"));
    //ui->deviceList->setItem(0, 1, new QTableWidgetItem("ready"));

    auto deviceTimer = new QTimer;
    auto boardcastTimer = new QTimer;

    auto boardcastReceiver = new QUdpSocket(this);
    auto boardcast = new QUdpSocket(this);
    auto connectHelper = new QUdpSocket(this);

    auto server = new QTcpServer(this);
    auto socket = new QTcpSocket(this);
    server->listen(QHostAddress::Any, 11452);

    //connect(ui->btn_initService, &QPushButton::clicked, [=]() {//初始化广播
    //    ui->btn_initService->setDisabled(true);
    //    boardcastTimer->start(250);
    //    deviceTimer->start(100);
    //    boardcastReceiver->bind(QHostAddress::Any, 11451, QUdpSocket::ShareAddress);
    //    });

    connect(ui->btn_recv, &QPushButton::clicked, [=]() {//初始化接收广播
        ui->btn_recv->setDisabled(true);
        deviceTimer->start(100);
        boardcastReceiver->bind(QHostAddress::Any, 11451, QUdpSocket::ShareAddress);
        });

    connect(ui->btn_start, &QPushButton::clicked, [=]() {//初始化发送广播
        boardcastTimer->start(250);
        connectHelper->bind(QHostAddress::Any, 11452, QUdpSocket::ShareAddress);
        ui->btn_start->setDisabled(true);
        });


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
                    ui->clientLog->append("已连接到" + QHostAddress(host.toIPv4Address()).toString() + "\n");
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

    connect(server, &QTcpServer::newConnection, [&]() {//接收设备连接
        socket = server->nextPendingConnection();
        ui->stackedWidget->setCurrentIndex(1);
        });
}

HotaruFileTransfer::~HotaruFileTransfer()
{
    delete ui;
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

