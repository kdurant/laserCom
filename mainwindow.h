#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

    struct RecvFile
    {
        QString    name;
        QFile      handle;
        QByteArray blockData;
        bool       isRunning;  // 是否正在接收数据
        qint32     size;       // 接收到的文件长度
        bool       isRecvBlock;
        QTimer *   blockTimer;
        QTimer *   fileStopTimer;
    };
    struct SendFile
    {
        bool    isRecvResponse;  // 是否收到响应数据
        bool    responseStatus;  // 收到接收机的状态
        bool    isTimeOut;       // 等待接收响应超时
        bool    sendOk;          // 收到接收机的正确响应，才算发送完成
        qint32  blockSize;
        qint32  prefixLen;  // 发送文件前缀长度
        qint32  requestBlockNumber;
        qint32  reSendCnt;
        QTimer *timer;
    };

    Ui::MainWindow *ui;
    QSettings *     configIni;
    QLabel *        statusLabel;
    const QString   softwareVer{SOFT_VERSION};

    QEventLoop *eventloop;
    QTimer *    recvFileWaitTimer;
    qint32      tcpPort;

    QString     deviceIP;
    QString     pcIP;
    QTcpSocket *tcpClient;
    qint32      tcpStatus;
    AT          at;
    bool        testStatus;

    struct RecvFile recvFile;
    OpStatus        opStatus;

    qint32 frameNumberOfTest;
    qint32 recvByteCnt;

    struct SendFile sendFile;
    QQueue<qint64>  request;
};
#endif  // MAINWINDOW_H
