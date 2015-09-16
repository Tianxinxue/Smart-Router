#include <QMessageBox>
#include "showwidget.h"
#include <QHostInfo>
ShowWidget::ShowWidget(QWidget *parent) : QWidget(parent)
{
    btnShowPic = new QPushButton("播放",this);
    btnConnect = new QPushButton("连接",this);
    labPic     = new QLabel;
    labPic->setMinimumSize(320,240);
    vLay = new QVBoxLayout (this);
    hLayBtn = new QHBoxLayout;
    hLaypic = new QHBoxLayout;
    hLayBtn->addWidget(btnConnect);
    hLayBtn->addWidget(btnShowPic);
    hLaypic->addWidget(labPic);

    vLay->addLayout(hLaypic);
    vLay->addLayout(hLayBtn);

    QObject::connect(btnShowPic,SIGNAL(clicked()),this,SLOT(onbtnShowPicclicked()));
    QObject::connect(btnConnect,SIGNAL(clicked()),this,SLOT(onbtnConnectclicked()));
}

void ShowWidget::onbtnShowPicclicked()
{

    //QMessageBox::warning(this,"warning","hello");
    QImage img;
    img.load("c:/tt.jpg");
    labPic->setPixmap(QPixmap::fromImage(img));
    tcpSocket->write("hh");
}

void ShowWidget::onbtnConnectclicked()
{
    serverIP =new QHostAddress();
    tcpSocket = new QTcpSocket(this);
    serverIP->setAddress("127.0.0.1");
    tcpSocket->connectToHost(*serverIP,1234);


}
