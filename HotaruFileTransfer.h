#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_HotaruFileTransfer.h"
#include <QUdpSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QTime>
#include <QTcpServer>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <thread>
#include <QDateTime>
#include <QDataStream>
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
    QString createLog(QString str);
    void deviceTimeout();
    void refreshTable();
    void updateProgressBar();
    inline void finishSendingFile();
    bool sendSingleFile(QString file, QString fileName);
    void sendFiles(QStringList files);
    void sendDirectory(QString dir);
    

    QList<ActiveDevice> devices;
    QTimer* deviceTimer;
    QTimer* boardcastTimer;
    QUdpSocket* boardcastReceiver;
    QUdpSocket* boardcast;
    QUdpSocket* connectHelper;
    QTcpServer* server;
    QTcpSocket* socket;
    QDataStream* outStream;
    QDataStream* inStream;

    int fileSize = 0;
    int bytesCompleted = 0;
    QFile operatingFile;


private:
    Ui::HotaruFileTransferClass *ui;
};
