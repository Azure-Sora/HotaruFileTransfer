#pragma once

#include <QObject>
#include <QHostAddress>
#include <QTimer>

class ActiveDevice  : public QObject
{
	Q_OBJECT

public:
	ActiveDevice(QObject *parent);
	ActiveDevice(QHostAddress addr, QString deviceName, QString status);
	ActiveDevice(const ActiveDevice& other);
	ActiveDevice& operator=(const ActiveDevice& other);
	bool operator==(const ActiveDevice& other) const;
	bool operator==(const QHostAddress& addr) const;
	~ActiveDevice();

	int lifeTime = 10;
	QHostAddress IPAddr;
	QString deviceName;
	QString status;
};
