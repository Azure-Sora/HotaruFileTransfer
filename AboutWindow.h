#pragma once

#include <QMainWindow>
#include "ui_AboutWindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class AboutWindowClass; };
QT_END_NAMESPACE

class AboutWindow : public QMainWindow
{
	Q_OBJECT

public:
	AboutWindow(QWidget *parent = nullptr);
	~AboutWindow();

private:
	Ui::AboutWindowClass *ui;
};
