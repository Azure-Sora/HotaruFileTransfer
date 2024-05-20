#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HotaruFileTransfer.h"
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QTime>
#include <QTcpServer>
#include <QMessageBox>
#include "ActiveDevice.h"
#include "NetworkUtil.h"

QT_BEGIN_NAMESPACE
namespace Ui { class HotaruFileTransferClass; };
QT_END_NAMESPACE

class HotaruFileTransfer : public QMainWindow
{
    Q_OBJECT

public:
    HotaruFileTransfer(QWidget *parent = nullptr);
    ~HotaruFileTransfer();
    void deviceTimeout();
    void refreshTable();

    QList<ActiveDevice> devices;
    QTimer* deviceTimer;
    QTimer* boardcastTimer;
    QUdpSocket* boardcastReceiver;
    QUdpSocket* boardcast;
    QUdpSocket* connectHelper;
    QTcpServer* server;
    QTcpSocket* socket;


private:
    Ui::HotaruFileTransferClass *ui;
};
