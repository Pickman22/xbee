#include <QCoreApplication>
#include <xbee.h>
#include <QThread>
#include <QDebug>
#include <string>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Xbee *bee = new Xbee("/dev/ttyUSB0", QSerialPort::Baud9600);
    bee->digitalOutput(pin_0, pin_on);
    bee->pinConfig(pin_0, digital_high);
    return a.exec();
}

