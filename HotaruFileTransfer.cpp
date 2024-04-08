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

    auto boardcastReceiver = new QUdpSocket(this);
    auto boardcast = new QUdpSocket(this);
    connect(ui->btn_receive, &QPushButton::clicked, [=]() {
        ui->btn_receive->setDisabled(true);
        boardcastReceiver->bind(11451, QUdpSocket::ShareAddress);
        });

    auto boardcastTimer = new QTimer;

    connect(ui->btn_startService, &QPushButton::clicked, [=]() {
        boardcastTimer->start(250);
        ui->btn_startService->setDisabled(true);
        });

    connect(boardcastTimer, &QTimer::timeout, [=]() {
        //auto time = QTime::currentTime();
        auto data = QByteArray("ready");
        //auto data = QByteArray(time.toString().toLocal8Bit());
        boardcast->writeDatagram(data.data(), QHostAddress::Broadcast, 11451);
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
            ui->deviceList->insertRow(0);
            ui->deviceList->setItem(0, 0, new QTableWidgetItem(QHostAddress(host.toIPv4Address()).toString()));
            ui->deviceList->setItem(0, 1, new QTableWidgetItem(QString(data)));
        }

        });
}

HotaruFileTransfer::~HotaruFileTransfer()
{
    delete ui;
}
