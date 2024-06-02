#include "AboutWindow.h"
#include <QPixmap>

AboutWindow::AboutWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::AboutWindowClass())
{
	ui->setupUi(this);
	setFixedSize(500, 500);
	ui->pic->setScaledContents(true);
	ui->pic->setFixedSize(300, 300);
	ui->pic->move(100, 50);
	ui->pic->setPixmap(QPixmap(":/HotaruFileTransfer/assets/about.jpg"));
}

AboutWindow::~AboutWindow()
{
	delete ui;
}
