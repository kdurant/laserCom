#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    tcpPort(17),
    tcpSocket(new QTcpSocket()),
    testStatus(false)
{
    ui->setupUi(this);

    configIni = new QSettings("./config.ini", QSettings::IniFormat);

    initParameter();
    initUI();
    initSignalSlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// configIni->value("System/oceanPort")
void MainWindow::initParameter()
{
    pcIP = read_ip_address();
    ui->lineEdit_pcIP->setText(pcIP);
}

//configIni->setValue("Laser/freq", 1111);
void MainWindow::saveParameter()
{
    configIni->setValue("System/RadarType", "land");
    configIni->setValue("Laser/freq", 1111);
}

void MainWindow::initUI()
{
    for(auto i : at.AT_query)
    {
        ui->comboBox_query->addItem(i.context);
    }
    for(auto i : at.AT_exe)
    {
        ui->comboBox_exe->addItem(i.context);
    }

    for(auto i : at.AT_setup)
    {
        ui->comboBox_setup->addItem(i.context);
    }
}

void MainWindow::initSignalSlot()
{
    QObject::connect(tcpSocket, &QTcpSocket::readyRead, this, [this]() {
        QByteArray buffer;
        buffer = tcpSocket->readAll();
        ui->plainTextEdit_at->appendPlainText(buffer);
    });
    //    QObject::connect(socket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);
    connect(ui->rbt_connect, &QRadioButton::toggled, this, [this]() {
        if(ui->rbt_connect->isChecked() == true)
        {
            deviceIP = ui->lineEdit_deviceIP->text();
            if(deviceIP.mid(0, deviceIP.lastIndexOf(".")) != pcIP.mid(0, pcIP.lastIndexOf(".")))
            {
                QMessageBox::warning(this, "警告", "网段不一致，请修改IP");
                ui->rbt_connect->setChecked(false);
                //                ui->rbt_disConnect->setChecked(true);
                return;
            }

            tcpSocket->connectToHost(deviceIP, tcpPort);

            if(tcpSocket->waitForConnected(3000))
            {
                qDebug("Connected!");
            }
            else
            {
                QEventLoop waitLoop;
                QTimer::singleShot(1000, &waitLoop, &QEventLoop::quit);
                waitLoop.exec();
                qDebug("Not Connected!");
                ui->rbt_connect->setChecked(false);
                //                ui->rbt_disConnect->setChecked(true);
            }
        }
        else
        {
            tcpSocket->disconnectFromHost();
        }
    });
    connect(ui->btn_querySend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_query->currentText() + "?\r\n";
        tcpSocket->write(data.toLatin1());
    });

    connect(ui->btn_exeSend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_query->currentText() + "\r\n";
        tcpSocket->write(data.toLatin1());
    });

    connect(ui->btn_setupSend, &QPushButton::pressed, this, [this]() {
        QString para = ui->lineEdit_setup->text();
        if(para.length() != 0)
        {
            QString data = "AT+" + ui->comboBox_query->currentText() + "=" + para + "\r\n";
            tcpSocket->write(data.toLatin1());
        }
    });
    connect(ui->btn_selectFile, &QPushButton::pressed, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, tr(""), "", tr("*"));  //选择路径
        if(filePath.size() == 0)
            return;
        ui->lineEdit_sendFile->setText(filePath);
    });

    connect(ui->btn_sendFile, &QPushButton::pressed, this, [this]() {
        QString filePath = ui->lineEdit_sendFile->text();
        if(filePath.isEmpty())
        {
            QMessageBox::warning(this, "warning", "请先选择文件");
            return;
        }

        QByteArray prefix(1446, 0xff);
        for(int i = 0; i < ui->lineEdit_prefixNumber->text().toInt(); i++)
        {
            tcpSocket->write(prefix);
        }

        ui->progressBar_sendFile->setMaximum(QFile(filePath).size());
        ui->progressBar_sendFile->setValue(0);

        QFile file(filePath);
        file.open(QIODevice::ReadOnly);

        char * buffer    = new char[1446];
        qint64 total_len = 0;
        while(!file.atEnd())
        {
            qint64 len = file.read(buffer, 1446);
            tcpSocket->write(buffer, len);
            total_len += len;
            ui->progressBar_sendFile->setValue(total_len);
        }
        qDebug() << total_len;
    });

    connect(ui->btn_stopTest, &QPushButton::pressed, this, [this]() {
        testStatus = false;
    });

    connect(ui->btn_sendTest, &QPushButton::pressed, this, [this]() {
        testStatus = true;

        QByteArray data(1446, 0);
        for(int i = 0; i < 1446; i++)
            data[i] = i % 256;

        while(true)
        {
            if(!testStatus)
                break;
            tcpSocket->write(data);
            QThread::usleep(1);
            QCoreApplication::processEvents();
        }
    });
}

QString MainWindow::read_ip_address()
{
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach(QHostAddress address, list)
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            if(address.toString().contains("127.0."))
            {
                continue;
            }
            return address.toString();
        }
    }
    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveParameter();
}
