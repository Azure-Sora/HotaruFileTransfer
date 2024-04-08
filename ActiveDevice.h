#pragma once

#include <QObject>
#include <QHostAddress>
#include <QTimer>

class ActiveDevice  : public QObject
{
	Q_OBJECT

public:
	ActiveDevice(QObject *parent);
	ActiveDevice(QHostAddress addr, QString status);
	ActiveDevice(const ActiveDevice& other);
	ActiveDevice& operator=(const ActiveDevice& other);
	~ActiveDevice();

	int lifeTime = 10;
	QHostAddress IPAddr;
	QString status;
};
