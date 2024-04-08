#include "ActiveDevice.h"

ActiveDevice::ActiveDevice(QObject *parent)
	: QObject(parent)
{}

ActiveDevice::ActiveDevice(QHostAddress addr, QString status)
	:
	IPAddr(addr),
	status(status)
{
	//QTimer *countDown;
	//connect(countDown, &QTimer::timeout, [&]() {
	//	this->lifeTime -= 1;
	//	//this->status = QString::number(lifeTime);
	//	if (this->lifeTime <= 0)
	//	{
	//		countDown->stop();
	//		delete countDown;
	//	}
	//	});
	//countDown->start(100);
}

ActiveDevice::ActiveDevice(const ActiveDevice& other) : IPAddr(other.IPAddr), status(other.status)
{
}

ActiveDevice& ActiveDevice::operator=(const ActiveDevice& other)
{
	this->IPAddr = other.IPAddr;
	this->status = other.status;
	return *this;
}

bool ActiveDevice::operator==(const ActiveDevice& other) const
{
	return this->IPAddr.isEqual(other.IPAddr);
}

bool ActiveDevice::operator==(const QHostAddress& addr) const
{
	return this->IPAddr.isEqual(addr);
}

ActiveDevice::~ActiveDevice()
{}
