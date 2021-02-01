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
    struct RecvFile
    {
        QString name;
        QFile   handle;
        bool    isRunning;   // 是否正在接收数据
        qint32  size;        // 接收到的文件长度
        qint32  headNumber;  // 用户校准的无效文件头包数
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

    qint32 frameNumberOfTest;
    qint32 recvByteCnt;
    qint32 lenPerPrefix;  // 发送文件前缀长度
};
#endif  // MAINWINDOW_H
