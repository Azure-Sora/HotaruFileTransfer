#include "HotaruFileTransfer.h"

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
    //ui->deviceList->setItem(0, 0, new QTableWidgetItem("114.514"));
    //ui->deviceList->setItem(0, 1, new QTableWidgetItem("ready"));

    auto deviceTimer = new QTimer;
    auto boardcastTimer = new QTimer;

    auto boardcastReceiver = new QUdpSocket(this);    
    connect(ui->btn_receive, &QPushButton::clicked, [=]() {
        ui->btn_receive->setDisabled(true);
        boardcastReceiver->bind(11451, QUdpSocket::ShareAddress);
        });

    connect(ui->btn_startService, &QPushButton::clicked, [=]() {
        boardcastTimer->start(250);
        deviceTimer->start(100);
        ui->btn_startService->setDisabled(true);
        });

    auto boardcast = new QUdpSocket(this);
    connect(boardcastTimer, &QTimer::timeout, [=]() {
        //auto time = QTime::currentTime();
        auto data = QByteArray("ready");
        //auto data = QByteArray(time.toString().toLocal8Bit());
        boardcast->writeDatagram(data.data(), QHostAddress::Broadcast, 11451);
        });

    connect(deviceTimer, &QTimer::timeout, [=]() {
        deviceTimeout();
        });

    connect(boardcastReceiver, &QUdpSocket::readyRead, [=]() {
        while (boardcastReceiver->hasPendingDatagrams())
        {
            QByteArray data;
            data.resize(boardcastReceiver->pendingDatagramSize());
            QHostAddress host;
            //quint16 port;
            boardcastReceiver->readDatagram(data.data(), data.size(), &host);

            //ui->deviceList->
            if (!deviceExists(QHostAddress(host.toIPv4Address())))
            {
                auto newdevice = ActiveDevice(QHostAddress(host.toIPv4Address()), QString(data));
                devices.append(newdevice);
                refreshTable();
            }
            

            //ui->deviceList->insertRow(0);
            //ui->deviceList->setItem(0, 0, new QTableWidgetItem(QHostAddress(host.toIPv4Address()).toString()));
            //ui->deviceList->setItem(0, 1, new QTableWidgetItem(QString(data)));
        }

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

bool HotaruFileTransfer::deviceExists(QHostAddress addr)
{
    for (auto &de : devices)
    {
        if (de.IPAddr.isEqual(addr)) return true;
    }
    return false;
}
