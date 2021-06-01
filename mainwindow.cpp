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
    testStatus(false)
{
    ui->setupUi(this);

    dispatch = new ProtocolDispatch();
    recvFile = new RecvFile();
    timer1s  = startTimer(1000);
    opStatus = IDLE;

    initParameter();
    initUI();
    initSignalSlot();
    userStatusBar();

    ui->label_changeLog->setText(CHANGELOG);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initParameter()
{
    QFileInfo fileInfo("./config.ini");
    if(!fileInfo.exists())
    {
        configIni = new QSettings("./config.ini", QSettings::IniFormat);
        configIni->setValue("System/mode", QString("debug_network # release, debug, debug_network"));
    }

    configIni = new QSettings("./config.ini", QSettings::IniFormat);

    if(configIni->value("System/mode").toString() == "debug_network")
        pcIP = "127.0.0.1";
    else
        pcIP = read_ip_address();

    if(configIni->value("System/mode").toString() == "debug_network")
    {
        ui->lineEdit_pcIP->setText("127.0.0.1");
        ui->lineEdit_deviceIP->setText("127.0.0.1");
    }
    else
    {
        ui->lineEdit_pcIP->setText(pcIP);
        ui->lineEdit_deviceIP->setText(deviceIP);
    }
}

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
    ui->label_statusLight->show();

    connect(ui->comboBox_query, &QComboBox::currentTextChanged, this, [this]()
            {
                if(ui->comboBox_query->currentText() == at.queryCommond[0].context)
                {
                    ui->comboBox_query->setToolTip(at.queryCommond[0].hint);
                }
                else if(ui->comboBox_query->currentText() == at.queryCommond[1].context)
                {
                    ui->comboBox_query->setToolTip(at.queryCommond[1].hint);
                }
            });
    connect(ui->comboBox_exe, &QComboBox::currentTextChanged, this, [this]()
            {
                if(ui->comboBox_exe->currentText() == at.exeCommand[0].context)
                {
                    ui->comboBox_exe->setToolTip(at.exeCommand[0].hint);
                }
            });

    connect(ui->comboBox_setup, &QComboBox::currentTextChanged, this, [this]()
            {
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
    connect(tcpClient, &QTcpSocket::readyRead, this, [this]()
            {
                QByteArray buffer;
                buffer = tcpClient->readAll();
                dispatch->parserFrame(buffer);
                //                statusLabel->setText("接收计数：" + QString::number(recvByteCnt).leftJustified(24, ' '));
            });

    connect(tcpClient, &QTcpSocket::disconnected, this, [this]()
            { ui->rbt_connect->setChecked(false); });
    connect(ui->rbt_connect, &QRadioButton::toggled, this, [this]()
            {
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

    connect(dispatch, &ProtocolDispatch::heartBeatReady, this, [this](quint32 cnt)
            {
                ui->label_heartBeatCnt->setText("心跳包序号：" + QString::number(cnt));
                QImage light_green(":qss/light_green.png");
                QImage light_gray(":qss/light_gray.png");
                ui->label_statusLight->setPixmap(QPixmap::fromImage(light_green));
                Common::sleepWithoutBlock(500);
                ui->label_statusLight->setPixmap(QPixmap::fromImage(light_gray));
            });

    connect(dispatch, &ProtocolDispatch::fileInfoReady, this, [this](QByteArray &data)
            { dispatch->encode(UserProtocol::RESPONSE_FILE_INFO, data); });
    connect(dispatch, &ProtocolDispatch::fileBlockReady, recvFile, &RecvFile::setNewData);

    connect(dispatch, &ProtocolDispatch::frameDataReady, this, [this](QByteArray &data)
            { tcpClient->write(data); });

    connect(ui->btn_querySend, &QPushButton::pressed, this, [this]()
            {
                QString data = "AT+" + ui->comboBox_query->currentText() + "?\r\n";
                tcpClient->write(data.toLatin1());
            });

    connect(ui->btn_exeSend, &QPushButton::pressed, this, [this]()
            {
                QString data = "AT+" + ui->comboBox_exe->currentText() + "\r\n";
                tcpClient->write(data.toLatin1());
            });

    connect(ui->btn_setupSend, &QPushButton::pressed, this, [this]()
            {
                QString para  = ui->lineEdit_setup->text();
                quint16 value = 0x00;

                if(ui->comboBox_setup->currentText() == "SDAC")
                {
                    if(ui->checkBox_autoSet01->isChecked())
                        value = para.toUInt(nullptr, 10);
                    else
                        value = para.toUInt(nullptr, 10) | 0x8000;
                    QString data = "AT+" + ui->comboBox_setup->currentText() + "=" + QString::number(value) + "\r\n";
                    tcpClient->write(data.toLatin1());
                    return;
                }
                if(para.length() != 0)
                {
                    QString data = "AT+" + ui->comboBox_setup->currentText() + "=" + para + "\r\n";
                    tcpClient->write(data.toLatin1());
                }
            });
    connect(ui->btn_selectFile, &QPushButton::pressed, this, [this]()
            {
                QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("*"));  //选择路径
                if(filePath.size() == 0)
                    return;
                ui->lineEdit_sendFile->setText(filePath);
            });

    connect(ui->btn_saveFile, &QPushButton::pressed, this, [this]()
            {
                QString name = QFileDialog::getOpenFileName(this, tr(""), "", tr("*"));
                ui->lineEdit_saveFileName->setText(name);
            });

    connect(ui->btn_sendFile, &QPushButton::pressed, this, [this]()
            {
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

                ui->progressBar_sendFile->setMaximum(QFile(filePath).size());
                ui->progressBar_sendFile->setValue(0);

                // 校验字段总共36*3=108Byte
                auto generateChecksum = [](char *data, qint32 len, qint32 number) -> QByteArray
                {
                    auto intToByte = [](int number) -> QByteArray
                    {
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
                    QByteArray tmp = res.append(QCryptographicHash::hash(send_data, QCryptographicHash::Md5));
                    res.append(tmp);
                    res.append(tmp);
                    return res;
                };
            });

    connect(ui->btn_stopTest, &QPushButton::pressed, this, [this]()
            { testStatus = false; });

    connect(ui->btn_sendTest, &QPushButton::pressed, this, [this]()
            {
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
            });

    connect(ui->btn_startRecvFile, &QPushButton::pressed, this, [this]()
            {
                if(ui->lineEdit_saveFileName->text().isEmpty())
                {
                    QMessageBox::warning(this, "warning", "请设置文件名");
                    return;
                }
            });

    connect(ui->btn_clearRecv, &QPushButton::pressed, this, [this]()
            {
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
    softwareLabel->setText("软件版本：" + QString(SOFT_VERSION));
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

void MainWindow::timerEvent(QTimerEvent *event)
{
    QByteArray data;
    if(timer1s == event->timerId())
    {
        if(tcpStatus == 0x01)
        {
            if(opStatus != SEND_FILE && opStatus != RECV_FILE)
            {
                heartBeatCnt++;
                data.append("Heart:");
                data.append(Common::int2ba(heartBeatCnt));
                dispatch->encode(UserProtocol::HEART_BEAT, data);
            }
        }
    }
}
