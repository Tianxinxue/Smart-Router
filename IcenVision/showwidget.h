#ifndef SHOWWIDGET
#define SHOWWIDGET
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTcpSocket>

class ShowWidget : public QWidget
{
    Q_OBJECT
private:
    QPushButton *btnShowPic;
    QPushButton *btnConnect;
    QLabel      *labPic;
    QHBoxLayout *hLaypic;
    QHBoxLayout *hLayBtn;
    QVBoxLayout *vLay;
    int port;
    QHostAddress *serverIP;
    QTcpSocket *tcpSocket;
public:
    explicit ShowWidget(QWidget *parent = 0);

signals:

public slots:
    void onbtnShowPicclicked();
    void onbtnConnectclicked();
};

#endif // SHOWWIDGET

