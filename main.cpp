#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("shell");
    QCoreApplication::setApplicationName("MyWindowsApp");
    MainWindow w;
    w.setWindowTitle("My Windows App");
    w.showMaximized();
    return QCoreApplication::exec();
}
