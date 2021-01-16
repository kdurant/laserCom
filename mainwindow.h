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

    qint32 tcpPort;

    QString     deviceIP;
    QString     pcIP;
    QTcpSocket *tcpSocket;
    qint32      tcpStatus;
    AT          at;
    bool       testStatus;
};
#endif  // MAINWINDOW_H
