#include "NetworkUtil.h"

QHostAddress NetworkUtil::getValidAddr()
{
    auto interfaces = QNetworkInterface::allInterfaces();

    for (auto& interface : interfaces)
    {
        if (interface.flags() & QNetworkInterface::IsRunning)
        {
            //qDebug() << interface.humanReadableName();
            if (interface.humanReadableName() == "以太网" || interface.humanReadableName() == "WLAN")
            {
                auto entrys = interface.addressEntries();
                for (auto entery : entrys)
                {
                    if (entery.ip().protocol() == QAbstractSocket::IPv4Protocol)
                    {
                        return entery.ip();
                    }
                }
            }
        }
    }

    return QHostAddress("0.0.0.0");
}
