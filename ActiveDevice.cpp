#include "ActiveDevice.h"

ActiveDevice::ActiveDevice(QObject *parent)
	: QObject(parent)
{}

ActiveDevice::ActiveDevice(QHostAddress addr, QString deviceName, QString status)
	:
	IPAddr(addr),
	status(status),
	deviceName(deviceName)
{
}

ActiveDevice::ActiveDevice(const ActiveDevice& other) : IPAddr(other.IPAddr), status(other.status), deviceName(other.deviceName)
{
}

ActiveDevice& ActiveDevice::operator=(const ActiveDevice& other)
{
	this->IPAddr = other.IPAddr;
	this->status = other.status;
	this->deviceName = other.deviceName;
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
