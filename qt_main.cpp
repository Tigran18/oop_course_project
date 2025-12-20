#include <QApplication>
#include "gui/MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("SlideShowGUI");
    QApplication::setOrganizationName("SlideShow");

    MainWindow w;
    w.resize(1100, 700);
    w.show();

    return app.exec();
}
