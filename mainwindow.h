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

    QString read_ip_address();

private:
    Ui::MainWindow *ui;
    QSettings *     configIni;

    QEventLoop *eventloop;
    QTimer *    recvFileWaitTimer;
    qint32      tcpPort;

    QString     deviceIP;
    QString     pcIP;
    QTcpSocket *tcpClient;
    qint32      tcpStatus;
    AT          at;
    bool        testStatus;
    QFile       saveFileHandle;
    QString     saveFileName;
    bool        isSaveFile;
    qint32      recvFileSize;  // 当前接收到文件的大小
    qint32      frameNumberOfTest;
};
#endif  // MAINWINDOW_H
