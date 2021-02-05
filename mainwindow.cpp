#include "mainwindow.h"
#include <qdebug.h>
#include "ui_mainwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), statusLabel(new QLabel()),
    //    softwareVer("0.05"),
    eventloop(new QEventLoop()),
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

    recvFile.isRunning   = false;
    recvFile.size        = 0;
    recvFile.isRecvBlock = false;
    recvFile.blockTimer  = new QTimer();
    recvFile.blockTimer->setInterval(10);
    recvFile.fileStopTimer = new QTimer();

    connect(ui->btn_saveFile, &QPushButton::pressed, this, [this]() {
        QString name = QFileDialog::getOpenFileName(this, tr(""), "", tr("*"));
        ui->lineEdit_saveFileName->setText(name);
    });

    connect(recvFile.blockTimer, &QTimer::timeout, this, [&]() {
        qint32 dataLen = recvFile.blockData.size();

        QByteArray validData     = recvFile.blockData.mid(0, dataLen - 36 * 3);
        QByteArray checksumField = recvFile.blockData.mid(dataLen - 36 * 3);
        QByteArray head          = checksumField.mid(0, 16);
        QByteArray blockNumber   = checksumField.mid(16, 4);
        QByteArray recv_md5      = checksumField.mid(20, 16);

        QByteArray expected_md5 = QCryptographicHash::hash(validData, QCryptographicHash::Md5);

        if(expected_md5 == recv_md5)
        {
            qDebug() << "receve data with correct md5";
            tcpClient->write(QByteArray(238, 'y'));
            recvFile.handle.write(validData);

            recvFile.size += validData.size();
        }
        else
        {
            tcpClient->write(QByteArray(238, 'n'));
            qDebug() << "receve data with wrong md5";
        }
        recvFile.blockData.clear();
        recvFile.blockTimer->stop();
    });

    sendFile.isRecvResponse = false;
    sendFile.responseStatus = false;
    sendFile.reSendCnt      = 0;
    sendFile.isTimeOut      = false;
    sendFile.timer          = new QTimer();

    ui->label_changeLog->setText(CHANGELOG);
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

    sendFile.prefixLen = configIni->value("SendFile/lenPerPrefix").toInt();
    sendFile.blockSize = configIni->value("SendFile/sendBlockSize").toInt();

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

        if(opStatus == OpStatus::SEND_FILE)
        {
            if(buffer.count('y') > 200)
            {
                sendFile.isRecvResponse = true;
                sendFile.responseStatus = true;
            }
            else
            {
                sendFile.isRecvResponse = true;
                sendFile.responseStatus = false;
            }
        }
        else if(opStatus == OpStatus::RECV_FILE)
        {
            recvFile.isRecvBlock = true;
            if(recvFile.isRecvBlock)
            {
                recvFile.blockTimer->setInterval(10);
                recvFile.blockTimer->start();
            }
            recvFile.blockData.append(buffer);

            recvFile.fileStopTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
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

            if(tcpClient->waitForConnected(100))
            {
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

        QByteArray prefix(sendFile.prefixLen, 0xff);
        //        for(int i = 0; i < ui->lineEdit_prefixNumber->text().toInt(); i++)
        //        {
        //            tcpClient->write(prefix);
        //        }

        ui->progressBar_sendFile->setMaximum(QFile(filePath).size());
        ui->progressBar_sendFile->setValue(0);

        opStatus           = OpStatus::SEND_FILE;
        sendFile.isTimeOut = false;

        QFile file(filePath);
        file.open(QIODevice::ReadOnly);

        char * buffer        = new char[sendFile.blockSize];
        qint32 normal_offset = 0;

        // 校验字段总共36*3=108Byte
        auto generateChecksum = [](char *data, qint32 len, qint32 number) -> QByteArray {
            auto intToByte = [](int number) -> QByteArray {
                QByteArray abyte0;
                abyte0.resize(4);
                abyte0[0] = (uchar)(0x000000ff & number);
                abyte0[1] = (uchar)((0x0000ff00 & number) >> 8);
                abyte0[2] = (uchar)((0x00ff0000 & number) >> 16);
                abyte0[3] = (uchar)((0xff000000 & number) >> 24);
                return abyte0;
            };

            QByteArray res;
            res.append(16, 0xfe);
            res.append(intToByte(number));

            QByteArray send_data{QByteArray::fromRawData(data, len)};
            res.append(QCryptographicHash::hash(send_data, QCryptographicHash::Md5));
            return res.append(res.append(res));
        };

        auto sendBlockData = [&](qint64 offset, qint32 block_number) -> qint64 {
            file.seek(offset);
            qint64 len = file.read(buffer, sendFile.blockSize);

            QByteArray send_data{QByteArray::fromRawData(buffer, len)};
            send_data.append(generateChecksum(buffer, len, block_number));
            for(int i = 0; i < ui->lineEdit_prefixNumber->text().toInt(); i++)
            {
                tcpClient->write(prefix);
            }
            tcpClient->write(send_data);
            //            tcpClient->flush();
            while(tcpClient->waitForBytesWritten())
                ;
            return len;
        };

        connect(sendFile.timer, &QTimer::timeout, this, [&]() {
            qDebug() << "Don't receive the correct response, generate a timeout signal";
            sendFile.isTimeOut = true;
        });

        while(normal_offset < file.size())
        {
            if(request.isEmpty() == false)  // 发送接收机重新请求数据
            {
                QThread::msleep(100);  // 解决tcp粘包问题
                qint64 request_offset = request.dequeue() * sendFile.blockSize;
                sendBlockData(request_offset, request_offset / sendFile.blockSize);
                QThread::msleep(100);
            }

            // 发送
            qint32 send_len = sendBlockData(normal_offset, normal_offset / sendFile.blockSize);
            qDebug("send %d Bytes", send_len);

            sendFile.timer->setInterval(1000);
            sendFile.timer->start();

            // 发送完一个数据块后，等待响应或者超时退出
            while(sendFile.isRecvResponse == false && sendFile.isTimeOut == false)
            {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            }
            sendFile.timer->stop();

            sendFile.isRecvResponse = false;
            sendFile.isTimeOut      = false;
            if(sendFile.responseStatus == true)
            {
                sendFile.responseStatus = false;
                normal_offset += send_len;
                qDebug("Send [success], file position: [%d]", normal_offset);
                continue;
            }
            else
            {
                sendFile.reSendCnt++;
            }

            if(sendFile.reSendCnt >= 3)
            {
                normal_offset += send_len;
                sendFile.reSendCnt = 0;
                qDebug("Send [failed], file position: [%d]", normal_offset);
            }
            ui->progressBar_sendFile->setValue(normal_offset);
        }
        opStatus = OpStatus::IDLE;
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
        recvFile.isRunning = true;
        opStatus           = MainWindow::RECV_FILE;

        recvFile.name = ui->lineEdit_saveFileName->text();
        recvFile.handle.setFileName(recvFile.name);
        recvFile.handle.open(QIODevice::WriteOnly);
        recvFile.size = 0;

        QEventLoop eventloop;
        connect(recvFile.fileStopTimer, SIGNAL(timeout()), &eventloop, SLOT(quit()));
        recvFile.fileStopTimer->setInterval(ui->lineEdit_saveTimeout->text().toInt() * 1000);
        recvFile.fileStopTimer->start();
        eventloop.exec();
        recvFile.isRunning = false;
        recvFile.handle.close();
        opStatus = MainWindow::IDLE;
        ui->btn_startRecvFile->setEnabled(true);
        ui->label_recvFileSize->setText("接收文件大小(Bytes):" +
                                        QString::number(recvFile.size));
    });

    connect(ui->btn_clearRecv, &QPushButton::pressed, this, [this]() {
        ui->plainTextEdit_at->clear();
        ui->progressBar_sendFile->setValue(0);
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
