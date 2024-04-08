#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HotaruFileTransfer.h"
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QTime>
#include "ActiveDevice.h"

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
    bool deviceExists(QHostAddress addr);

    QList<ActiveDevice> devices;


private:
    Ui::HotaruFileTransferClass *ui;
};
