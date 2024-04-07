#include "HotaruFileTransfer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    HotaruFileTransfer w;
    w.show();
    return a.exec();
}
