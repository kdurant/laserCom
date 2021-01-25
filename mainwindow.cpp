#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    statusLabel(new QLabel()),
    softwareVer("0.05"),
    eventloop(new QEventLoop()),
    recvFileWaitTimer(new QTimer()),
    tcpPort(17),
    tcpClient(new QTcpSocket()),
    tcpStatus(0),
    testStatus(false),
    recvByteCnt(0)
{
    ui->setupUi(this);

    configIni = new QSettings("./config.ini", QSettings::IniFormat);

    initParameter();
    initUI();
    initSignalSlot();
    userStatusBar();

    recvFile.isRunning = false;
    recvFile.size      = 0;

    ui->label_changeLog->setText("v0.05: 测试数据设置为递增ascii字符");
}

MainWindow::~MainWindow()
{
    delete ui;
}

// configIni->value("System/oceanPort")
void MainWindow::initParameter()
{
    if(configIni->value("System/mode").toString() == "debug")
        pcIP = "127.0.0.1";
    else
        pcIP = read_ip_address();
    ui->lineEdit_pcIP->setText(pcIP);

    lenPerPrefix = configIni->value("SendFile/lenPerPrefix").toInt();

    deviceIP          = configIni->value("System/deviceIP").toString();
    frameNumberOfTest = configIni->value("System/frameNumber").toInt();
    if(frameNumberOfTest > 50 || frameNumberOfTest <= 0)
        QMessageBox::warning(this, "warning", " 0 < frameNumberOfTest < 50");
}

//configIni->setValue("Laser/freq", 1111);
void MainWindow::saveParameter()
{
    //    configIni->setValue("System/deviceIP", ui->lineEdit_deviceIP->text());
    //    configIni->setValue("Laser/freq", 1111);
}

void MainWindow::initUI()
{
    setWindowTitle("激光通信测试软件");
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
        recvByteCnt += buffer.size();
        statusLabel->setText("接收计数：" + QString::number(recvByteCnt).leftJustified(24, ' '));

        if(recvFile.isRunning)
        {
            recvFile.handle.write(buffer);
            recvFile.size += buffer.size();
            recvFileWaitTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
        }
        else
        {
            QByteArray judge(10, 0xff);
            if(buffer.mid(0, 10) == judge)
            {
                recvFile.headNumber++;
                return;
            }
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

        QByteArray prefix(lenPerPrefix, 0xff);
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

        QByteArray data(1446, 0);
        for(int i = 0; i < data.size(); i++)
        {
            uint8_t pad = static_cast<uint8_t>(i / 238) + 0x31;
            data[i]     = pad;
        }
        qint64 sendCnt = 0;

        int number = ui->lineEdit_testFrameNumber->text().toInt();
        if(number != 0)
        {
            for(int i = 0; i < number; i++)
            {
                tcpClient->write(data);
                sendCnt += data.size();
                ui->label_sendCnt->setText("发送数据：" + QString::number(sendCnt) + "Bytes/" +
                                           QString::number(sendCnt / 1024.0 / 1024, 10, 3) + "Mb/" +
                                           QString::number(sendCnt / 1024.0 / 1024 / 1024, 10, 3) + "Gb");
            }
            return;
        }
        QByteArray data1(238 * 6 * frameNumberOfTest, 0);
        for(int i = 0; i < data.size(); i++)
        {
            uint8_t pad = static_cast<uint8_t>(i / 238) + 0x31;
            data[i]     = pad;
        }
        while(true)
        {
            if(!testStatus)
                break;
            tcpClient->write(data1);
            sendCnt += data1.size();
            ui->label_sendCnt->setText("发送数据：" + QString::number(sendCnt) + "Bytes/" +
                                       QString::number(sendCnt / 1024.0 / 1024, 10, 3) + "Mb/" +
                                       QString::number(sendCnt / 1024.0 / 1024 / 1024, 10, 3) + "Gb");
            QThread::usleep(1);
            QCoreApplication::processEvents();
        }
    });

    connect(ui->btn_saveFile, &QPushButton::pressed, this, [this]() {
        recvFile.name = QFileDialog::getOpenFileName(this, tr(""), "", tr("*"));
        ui->lineEdit_saveFileName->setText(recvFile.name);
    });

    connect(ui->btn_startRecvFile, &QPushButton::pressed, this, [this]() {
        if(ui->lineEdit_saveFileName->text().isEmpty())
        {
            QMessageBox::warning(this, "warning", "请设置文件名");
            return;
        }
        if(tcpStatus == 0x00)
        {
            QMessageBox::warning(this, "warning", "请检查TCP是否连接");
            return;
        }
        ui->btn_startRecvFile->setEnabled(false);
        recvFile.isRunning  = true;
        recvFile.headNumber = 0;

        recvFile.handle.setFileName(recvFile.name);
        recvFile.handle.open(QIODevice::WriteOnly);
        recvFile.size = 0;

        QEventLoop eventloop;
        connect(recvFileWaitTimer, SIGNAL(timeout()), &eventloop, SLOT(quit()));
        recvFileWaitTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
        recvFileWaitTimer->start();
        eventloop.exec();
        recvFile.isRunning = false;
        recvFile.handle.close();
        ui->btn_startRecvFile->setEnabled(true);
        ui->label_recvFileSize->setText("接收文件大小(Bytes):" +
                                        QString::number(recvFile.size) +
                                        "\n无效文件头个数:" +
                                        QString::number(recvFile.headNumber));
    });

    connect(ui->btn_clearRecv, &QPushButton::pressed, this, [this]() {
        ui->plainTextEdit_at->clear();
        recvByteCnt = 0;
        statusLabel->setText("接收计数：" + QString::number(recvByteCnt).leftJustified(24, ' '));
    });
}

void MainWindow::userStatusBar()
{
    statusLabel->setText("接收计数：" + QString::number(recvByteCnt).leftJustified(24, ' '));
    ui->statusbar->addPermanentWidget(statusLabel);

    QLabel *softwareLabel = new QLabel();
    softwareLabel->setText("软件版本：" + softwareVer);
    ui->statusbar->addPermanentWidget(softwareLabel);
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
