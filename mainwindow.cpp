#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), eventloop(new QEventLoop()), recvFileWaitTimer(new QTimer()), tcpPort(17), tcpClient(new QTcpSocket()), testStatus(false), isSaveFile(false)
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

    deviceIP          = configIni->value("System/deviceIP").toString();
    frameNumberOfTest = configIni->value("System/frameNumber").toInt();
    if(frameNumberOfTest > 50 || frameNumberOfTest <= 0)
        QMessageBox::warning(this, "warning", " 0 < frameNumberOfTest < 50");
}

//configIni->setValue("Laser/freq", 1111);
void MainWindow::saveParameter()
{
    configIni->setValue("System/deviceIP", ui->lineEdit_deviceIP->text());
    //    configIni->setValue("Laser/freq", 1111);
}

void MainWindow::initUI()
{
    for(auto i : at.queryCommond)
    {
        ui->comboBox_query->addItem(i.context);
    }
    ui->comboBox_query->setToolTip(at.queryCommond[0].hint);
    for(auto i : at.exeCommand)
    {
        ui->comboBox_exe->addItem(i.context);
    }
    ui->comboBox_exe->setToolTip(at.exeCommand[0].hint);

    for(auto i : at.setupCommand)
    {
        ui->comboBox_setup->addItem(i.context);
    }
    ui->comboBox_setup->setToolTip(at.setupCommand[0].hint);

    ui->progressBar_sendFile->setValue(0);
    ui->lineEdit_deviceIP->setText(deviceIP);

    connect(ui->comboBox_query, &QComboBox::currentTextChanged, this, [this]() {
        if(ui->comboBox_query->currentText() == at.queryCommond[0].context)
        {
            ui->comboBox_query->setToolTip(at.queryCommond[0].hint);
        }
        else if(ui->comboBox_query->currentText() == at.queryCommond[1].context)
        {
            ui->comboBox_query->setToolTip(at.queryCommond[1].hint);
        }
    });
    connect(ui->comboBox_exe, &QComboBox::currentTextChanged, this, [this]() {
        if(ui->comboBox_exe->currentText() == at.exeCommand[0].context)
        {
            ui->comboBox_exe->setToolTip(at.exeCommand[0].hint);
        }
    });

    connect(ui->comboBox_setup, &QComboBox::currentTextChanged, this, [this]() {
        if(ui->comboBox_setup->currentText() == at.setupCommand[0].context)
        {
            ui->lineEdit_setup->setText("900");
            ui->comboBox_setup->setToolTip(at.setupCommand[0].hint);
        }
        else if(ui->comboBox_setup->currentText() == at.setupCommand[1].context)
        {
            ui->lineEdit_setup->setText("5");
            ui->comboBox_setup->setToolTip(at.setupCommand[1].hint);
        }
    });
}

void MainWindow::initSignalSlot()
{
    connect(tcpClient, &QTcpSocket::readyRead, this, [this]() {
        QByteArray buffer;
        buffer = tcpClient->readAll();

        if(isSaveFile)
        {
            saveFileHandle.write(buffer);
            recvFileWaitTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
        }
        else
        {
            if(ui->plainTextEdit_at->toPlainText().length() > 1024 * 1024)
                ui->plainTextEdit_at->clear();
            ui->plainTextEdit_at->appendPlainText(buffer);
        }
    });
    connect(tcpClient, &QTcpSocket::disconnected, this, [this]() {
        ui->rbt_connect->setChecked(false);
    });
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

            tcpClient->connectToHost(deviceIP, tcpPort);

            ui->rbt_connect->setChecked(true);
            //            tcpSocket->connectToHost("127.0.0.1", tcpPort);

            if(tcpClient->waitForConnected(100))
            {
                qDebug("Connected!");
                tcpStatus = 0x01;
            }
            else
            {
                QEventLoop waitLoop;
                QTimer::singleShot(1000, &waitLoop, &QEventLoop::quit);
                ui->rbt_connect->setChecked(true);
                waitLoop.exec();
                ui->rbt_connect->setChecked(false);
                tcpStatus = 0x00;
            }
        }
        else
        {
            tcpClient->disconnectFromHost();
            tcpStatus = 0x00;
        }
    });
    //    connect(tcpClient, &QTcpSocket::stateChanged, this, [this](QAbstractSocket::SocketState socketState) {
    //        qDebug() << "------" << socketState;
    //    });

    connect(ui->btn_querySend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_query->currentText() + "?\r\n";
        tcpClient->write(data.toLatin1());
    });

    connect(ui->btn_exeSend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_exe->currentText() + "\r\n";
        tcpClient->write(data.toLatin1());
    });

    connect(ui->btn_setupSend, &QPushButton::pressed, this, [this]() {
        QString para = ui->lineEdit_setup->text();
        if(para.length() != 0)
        {
            QString data = "AT+" + ui->comboBox_setup->currentText() + "=" + para + "\r\n";
            tcpClient->write(data.toLatin1());
        }
    });
    connect(ui->btn_selectFile, &QPushButton::pressed, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("*"));  //选择路径
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

        if(tcpStatus == 0x00)
        {
            QMessageBox::warning(this, "warning", "请检查TCP是否连接");
            return;
        }

        QByteArray prefix(1446, 0xff);
        for(int i = 0; i < ui->lineEdit_prefixNumber->text().toInt(); i++)
        {
            tcpClient->write(prefix);
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
            tcpClient->write(buffer, len);
            total_len += len;
            ui->progressBar_sendFile->setValue(total_len);
        }
        qDebug() << total_len;
    });

    connect(ui->btn_stopTest, &QPushButton::pressed, this, [this]() {
        testStatus = false;
    });

    connect(ui->btn_sendTest, &QPushButton::pressed, this, [this]() {
        if(tcpStatus == 0x00)
        {
            QMessageBox::warning(this, "warning", "请检查TCP是否连接");
            return;
        }
        testStatus = true;

        QByteArray data(1446 * frameNumberOfTest, 0);
        for(int i = 0; i < data.size(); i++)
            data[i] = i % 256;
        qint64 sendCnt = 0;
        while(true)
        {
            if(!testStatus)
                break;
            tcpClient->write(data);
            sendCnt += data.size();
            ui->label_sendCnt->setText("发送数据：" + QString::number(sendCnt) + "Bytes/" +
                                       QString::number(sendCnt / 1024.0 / 1024, 10, 3) + "Mb/" +
                                       QString::number(sendCnt / 1024.0 / 1024 / 1024, 10, 3) + "Gb");
            QThread::usleep(1);
            QCoreApplication::processEvents();
        }
    });

    connect(ui->btn_saveFile, &QPushButton::pressed, this, [this]() {
        saveFileName = QFileDialog::getOpenFileName(this, tr(""), "", tr("*"));
        ui->lineEdit_saveFileName->setText(saveFileName);
    });

    connect(ui->btn_startRecvFile, &QPushButton::pressed, this, [this]() {
        if(ui->lineEdit_saveFileName->text().isEmpty())
        {
            QMessageBox::warning(this, "warning", "请设置文件名");
            return;
        }
        ui->btn_startRecvFile->setEnabled(false);
        isSaveFile = true;

        saveFileHandle.setFileName(saveFileName);
        saveFileHandle.open(QIODevice::WriteOnly);

        QEventLoop eventloop;
        connect(recvFileWaitTimer, SIGNAL(timeout()), &eventloop, SLOT(quit()));
        recvFileWaitTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
        recvFileWaitTimer->start();
        eventloop.exec();
        isSaveFile = false;
        saveFileHandle.close();
        ui->btn_startRecvFile->setEnabled(true);
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
