#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HotaruFileTransfer.h"
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class HotaruFileTransferClass; };
QT_END_NAMESPACE

class HotaruFileTransfer : public QMainWindow
{
    Q_OBJECT

public:
    HotaruFileTransfer(QWidget *parent = nullptr);
    ~HotaruFileTransfer();

private:
    Ui::HotaruFileTransferClass *ui;
};
