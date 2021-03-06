#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <algorithm>

#include <QMainWindow>
#include <QtCore>
#include <QSettings>
#include <QString>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QCryptographicHash>

#include "at.h"
#include "common.h"
#include "protocol.h"
#include "ProtocolDispatch.h"
#include "sendFile.h"
#include "recvFile.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
protected:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initParameter();
    void saveParameter();

    void initUI();
    void initSignalSlot();
    void userStatusBar();

    QString read_ip_address();

private:
    enum OpStatus
    {
        IDLE = 0x00,
        TEST_MODE,
        SEND_FILE,
        RECV_FILE,
    };
    struct _sys_para_
    {
        QString pcIP;
        QString mode;
        int     blockSize;
        int     repeatNum;
    } sysPara{
        "", "", 0, 3};

    Ui::MainWindow *ui;
    QSettings *     configIni;
    QLabel *        statusLabel;
    qint32          timer1s;

    QEventLoop *eventloop;
    qint32      tcpPort;

    QString     deviceIP{"192.168.1.10"};
    QTcpSocket *tcpClient;
    qint32      tcpStatus;
    AT          at;
    bool        testStatus{false};

    ProtocolDispatch *dispatch;
    RecvFile *        recvFlow;
    SendFile *        sendFlow;
    quint32           heartBeatCnt{0};
    quint32           recvByteCnt{0};
    QFile             userFile;
    QVector<bool>     sendFileBlockStatus{false};

    OpStatus opStatus;
};
#endif  // MAINWINDOW_H
