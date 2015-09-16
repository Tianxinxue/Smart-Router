
#include <QApplication>
#include "showwidget.h"

int main(int argc,char **argv)
{
    QApplication a(argc,argv);
    ShowWidget s;
    s.show();
    return a.exec();

}
