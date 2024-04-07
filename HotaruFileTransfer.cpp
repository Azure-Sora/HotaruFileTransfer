#include "HotaruFileTransfer.h"

HotaruFileTransfer::HotaruFileTransfer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::HotaruFileTransferClass())
{
    ui->setupUi(this);
}

HotaruFileTransfer::~HotaruFileTransfer()
{
    delete ui;
}
