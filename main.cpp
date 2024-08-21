#include <QLabel>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLabel *label = new QLabel("Hello world ssh!");
    label->show();

    return a.exec();
}