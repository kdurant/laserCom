#include "mainwindow.h"
#include <qdebug.h>
#include "ui_mainwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    statusLabel(new QLabel()),
    //    softwareVer("0.05"),
    eventloop(new QEventLoop()),
    tcpPort(17),
    tcpClient(new QTcpSocket()),
    tcpStatus(0),
    testStatus(false)
{
    ui->setupUi(this);

    dispatch = new ProtocolDispatch();
    recvFlow = new RecvFile();
    sendFlow = new SendFile();
    timer1s  = startTimer(1000);
    thread   = new QThread();
    opStatus = IDLE;
    tcpClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    audioRecord = new AudioRecord();
    player      = new QMediaPlayer();
    playlist    = new QMediaPlaylist();
    playlist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
    player->setPlaylist(playlist);

    cameraViewfinder = new QCameraViewfinder();
    ui->hlayout->addWidget(cameraViewfinder);

    camera = new QCamera();
    camera->setCaptureMode(QCamera::CaptureStillImage);
    camera->setViewfinder(cameraViewfinder);

    cameraImageCapture = new QCameraImageCapture(camera);
    cameraImageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);
    cameraTimer = new QTimer();

    dispatch->moveToThread(thread);
    connect(dispatch, &ProtocolDispatch::readyRead, dispatch, &ProtocolDispatch::parserFrame);
    thread->start();

    initParameter();
    initUI();
    initSignalSlot();
    userStatusBar();

    qDebug() << "main thread id = " << QThread::currentThreadId();
    //    ui->label_changeLog->setText(CHANGELOG);
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
        QFile file("./config.ini");
        file.open(QIODevice::WriteOnly);
        file.write("[System]\r\n");
        file.write("; release, debug, debug_network\r\n");
        file.write("mode = debug_network\r\n\r\n");
        file.write("[SendFile]\r\n");
        file.write("; 1M= 1048576, 512K = 524288, 256K = 262144, 128K = 131072, 64K = 65536, 32K = 32768\r\n");
        file.write("blockSize = 8192\r\n");
        file.write("repeatNum = 3\r\n");
        file.write("; the interval time of sending block data\r\n");
        file.write("blockIntervalTime = 5\r\n");
        file.write("cycleIntervalTime = 100\r\n");
        file.close();
    }

    configIni                 = new QSettings("./config.ini", QSettings::IniFormat);
    sysPara.mode              = configIni->value("System/mode").toString();
    sysPara.blockSize         = configIni->value("SendFile/blockSize").toUInt();
    sysPara.repeatNum         = configIni->value("SendFile/repeatNum").toUInt();
    sysPara.blockIntervalTime = configIni->value("SendFile/blockIntervalTime").toUInt();
    sysPara.cycleIntervalTime = configIni->value("SendFile/cycleIntervalTime").toUInt();

    if(sysPara.mode == "debug_network")
        sysPara.pcIP = "127.0.0.1";
    else
        sysPara.pcIP = read_ip_address();

    if(sysPara.mode == "debug_network")
    {
        ui->lineEdit_pcIP->setText("127.0.0.1");
        ui->lineEdit_deviceIP->setText("127.0.0.1");
    }
    else
    {
        ui->lineEdit_pcIP->setText(sysPara.pcIP);
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

    QDir("cache").removeRecursively();
    QDir().mkpath("cache/master");
    QDir().mkpath("cache/slave");
}

void MainWindow::initSignalSlot()
{
    connect(tcpClient, &QTcpSocket::readyRead, this, [this]() {
        QByteArray buffer;
        buffer = tcpClient->readAll();
        dispatch->setNewData(buffer);

        testStatus = true;
        //                statusLabel->setText("接收计数：" + QString::number(recvByteCnt).leftJustified(24, ' '));
    });

    connect(tcpClient, &QTcpSocket::disconnected, this, [this]() {
        ui->rbt_connect->setChecked(false);
    });
    connect(ui->rbt_connect, &QRadioButton::toggled, this, [this]() {
        if(ui->rbt_connect->isChecked() == true)
        {
            deviceIP = ui->lineEdit_deviceIP->text();
            if(deviceIP.mid(0, deviceIP.lastIndexOf(".")) != sysPara.pcIP.mid(0, sysPara.pcIP.lastIndexOf(".")))
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

    connect(dispatch, &ProtocolDispatch::frameDataReady, this, [this](QByteArray &data) {
        tcpClient->write(data);
        tcpClient->waitForBytesWritten();
    });

    // connect(dispatch, &ProtocolDispatch::errorDataReady, this, [this](QByteArray &data) {
    // qDebug() << data;
    // });

    connect(dispatch, &ProtocolDispatch::heartBeatReady, this, [this](quint32 cnt) {
        ui->label_heartBeatCnt->setText("心跳包序号：" + QString::number(cnt));
        QImage light_green(":qss/light_green.png");
        QImage light_gray(":qss/light_gray.png");
        ui->label_statusLight->setPixmap(QPixmap::fromImage(light_green));
        Common::sleepWithoutBlock(500);
        ui->label_statusLight->setPixmap(QPixmap::fromImage(light_gray));
    });

    /*
     接收端对协议的处理
     */

    // 1. 从机收到主机发送的信号（0x20）
    // 2. 主机同样会接收到相似的信号(0x21)
    // 但处理方法不一样，所以信号名要加以区分
    // 收到正确的文件信息，立刻响应发送端
    connect(dispatch, &ProtocolDispatch::slaveFileInfoReady, this, [this](QByteArray data) {
        recvFlow->setFileInfo(data);
        opStatus = RECV_FILE;

        if(!recvFlow->getFileName().endsWith("jpg"))
        {
            dispatch->encode(UserProtocol::RESPONSE_FILE_INFO, data);
        }
    });

    // 接收端发送的响应由于TCP流的关系，发送方会延迟收到，导致已经正确的数据，重复发送
    connect(recvFlow, &RecvFile::fileBlockReady, this, [this](QString fileName, quint32 blockNo, quint32 validLen, QByteArray &recvData) {
        qInfo() << fileName << " :receive blockNo = " << blockNo;

        if(!fileName.endsWith("jpg"))
        {
            QByteArray frame = recvFlow->packResponse(fileName, blockNo, validLen);
            dispatch->encode(UserProtocol::RESPONSE_FILE_DATA, frame);
        }

        qint64 offset = 0;
        if(validLen != recvFlow->getBlockSize(fileName))
            offset = blockNo * recvFlow->getBlockSize(fileName);
        else
            offset = blockNo * validLen;

        // 如果数据块没有被标记，则将数据写入文件
        if(recvFlow->getBlockStatus(fileName, blockNo) == false)
        {
            qInfo() << fileName << " : write file block at : " << offset;
            recvFlow->recvList[fileName].file.seek(offset);
            recvFlow->recvList[fileName].file.write(recvData);
            recvFlow->setBlockStatus(fileName, blockNo);
        }

        qInfo() << fileName
                << ": recvFlow->getAllBlockNumber(fileName) = " << recvFlow->getAllBlockNumber(fileName)
                << ", recvFlow->getBlockSuccessNumber(fileName) = " << recvFlow->getBlockSuccessNumber(fileName);
        if(recvFlow->isRecvAllBlock(fileName))
        {
            if(recvFlow->recvList[fileName].file.handle() == -1)  // 文件已经关闭
                return;

            qInfo() << fileName << ": receive all file blocks, close file";
            recvFlow->recvList[fileName].file.close();
            recvFlow->eraseFileNode(fileName);

            ui->textEdit_recv->append("<font color=blue>[Receive] " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss") + "</font>");

            QString path = "cache/slave/" + recvFlow->getFileName();
            if(path.toLower().endsWith("chat"))
            {
                QFile file(path);
                file.open(QIODevice::ReadOnly);

                ui->textEdit_recv->append(file.readAll());
                file.close();
            }
            else if(path.toLower().endsWith("png"))
            {
                ui->label_recvFile->setPixmap(QPixmap(path));
                ui->textEdit_recv->append("received file: " + recvFlow->getFileName() + ". 请打开图片界面查看");
            }

            else if(path.toLower().endsWith("jpg"))
            {
                QElapsedTimer timer;
                timer.start();
                ui->textEdit_recv->append("received file: " + recvFlow->getFileName() + ".");
                ui->label_vedioShow->setPixmap(QPixmap(path));
                qInfo() << "Display picture elapsed(ms) : " << timer.elapsed();
            }

            else if(path.toLower().endsWith("wav"))
            {
                ui->textEdit_recv->append("received file: " + path + ".");
                playlist->clear();
                playlist->addMedia(QUrl::fromLocalFile(recvFlow->getFileName()));
                player->play();
            }
            else
            {
                ui->textEdit_recv->append("received file: " + path + ".");
            }
            opStatus = IDLE;
            qInfo() << "All file blocks are received!";
        }
    });
    // 收到文件块数据，发送接收文件模块处理
    connect(dispatch, &ProtocolDispatch::slaveFileBlockReady, recvFlow, &RecvFile::paserNewData);

    //
    connect(ui->btn_querySend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_query->currentText() + "?\r\n";
        tcpClient->write(data.toLatin1());
    });

    connect(ui->btn_exeSend, &QPushButton::pressed, this, [this]() {
        QString data = "AT+" + ui->comboBox_exe->currentText() + "\r\n";
        tcpClient->write(data.toLatin1());
    });

    connect(ui->btn_setupSend, &QPushButton::pressed, this, [this]() {
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
    connect(ui->btn_selectFile, &QPushButton::pressed, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("*"));  //选择路径
        if(filePath.size() == 0)
            return;
        ui->lineEdit_sendFile->setText(filePath);
    });

    /*
     发送端处理流程
     */
    connect(this, &MainWindow::receivedNewBlock, sendFlow, &SendFile::getNewBlock);
    connect(dispatch, &ProtocolDispatch::masterFileInfoReady, sendFlow, &SendFile::setNewData);
    connect(dispatch, &ProtocolDispatch::masterFileBlockReady, this, [this](QByteArray data) {
        int     offset = data.indexOf('?');
        QString name   = data.mid(0, offset);
        offset++;
        int curretFileBlock = Common::ba2int(data.mid(offset + 5, 4));
        sendFlow->setBlockStatus(name, curretFileBlock, true);
        emit receivedNewBlock();
        qDebug() << "receive blockNo is: " << curretFileBlock;
    });

    // connect(sendFlow, &SendFile::sendDataReady, dispatch, &ProtocolDispatch::encode);
    connect(sendFlow, &SendFile::sendDataReady, this, [this](qint32 command, QByteArray &data) {
        dispatch->encode(command, data);
    });

    connect(ui->btn_sendFile, &QPushButton::pressed, this, [this]() {
        opStatus         = SEND_FILE;
        QString filePath = ui->lineEdit_sendFile->text();
        if(sysPara.mode == "debug_network")
            filePath = "ui_mainwindow.h";

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
        sendFlow->setFileName(filePath);
        filePath = Common::getFileNameFromFullPath(filePath);
        sendFlow->setFileBlockSize(filePath, sysPara.blockSize);
        if(sendFlow->send(filePath, sysPara.blockIntervalTime, sysPara.repeatNum) == false)
            QMessageBox::warning(this, "warning", "发送文件失败");

        ui->textEdit_recv->append("<font color=red>[Sender] " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss") + "</font>");
        ui->textEdit_recv->append(filePath);

        opStatus = IDLE;
    });

    connect(ui->btn_sendText, &QPushButton::pressed, this, [this]() {
        if(ui->textEdit_send->toPlainText().length() == 0)
            return;
        QFile file("cache/master/tmpFile.chat");
        file.open(QIODevice::WriteOnly);
        file.write(ui->textEdit_send->toPlainText().toStdString().data());
        file.close();

        ui->textEdit_recv->append("<font color=red>[Sender] " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss") + "</font>");
        ui->textEdit_recv->append(ui->textEdit_send->toPlainText());

        opStatus = SEND_FILE;
        sendFlow->setFileName("cache/master/tmpFile.chat");
        sendFlow->setFileBlockSize("tmpFile.chat", sysPara.blockSize);

        if(sendFlow->send("tmpFile.chat", sysPara.blockIntervalTime, sysPara.repeatNum) == false)
            QMessageBox::warning(this, "warning", "信息文件失败");

        ui->textEdit_send->clear();
        opStatus = IDLE;
    });

    connect(ui->btn_sendTest, &QPushButton::pressed, this, [this]() {
        if(tcpStatus == 0x00)
        {
            QMessageBox::warning(this, "warning", "请检查TCP是否连接");
            return;
        }

        QByteArray data    = QByteArray(1411, 0x11);
        int        number  = ui->lineEdit_testFrameNumber->text().toInt();
        int        sendCnt = 0;
        if(number != 0)
        {
            for(int i = 0; i < number; i++)
            {
                dispatch->encode(UserProtocol::SET_TEST_PATTERN, data);
                sendCnt += 1446;
                ui->label_sendCnt->setText("发送数据：" + QString::number(sendCnt) + "Bytes/" +
                                           QString::number(sendCnt / 1024.0 / 1024, 10, 3) + "Mb/" +
                                           QString::number(sendCnt / 1024.0 / 1024 / 1024, 10, 3) + "Gb");
            }
            return;
        }
        testStatus = true;
        while(true)
        {
            if(!testStatus)
                break;
            dispatch->encode(UserProtocol::SET_TEST_PATTERN, data);
            sendCnt += 1446;
            ui->label_sendCnt->setText("发送数据：" + QString::number(sendCnt) + "Bytes/" +
                                       QString::number(sendCnt / 1024.0 / 1024, 10, 3) + "Mb/" +
                                       QString::number(sendCnt / 1024.0 / 1024 / 1024, 10, 3) + "Gb");
            Common::sleepWithoutBlock(5);
        }
    });

    connect(ui->btn_stopTest, &QPushButton::pressed, this, [this]() {
        testStatus = false;
    });

    //********************语音相关操作*************************************
    connect(ui->btn_audioStart, &QPushButton::pressed, this, [this]() {
        ui->btn_audioStart->setEnabled(false);
        currentAudioFile = audioRecord->configSaveAudio();
        audioRecord->record();
    });
    connect(ui->btn_audioStop, &QPushButton::pressed, this, [this]() {
        audioRecord->stop();
        ui->btn_audioStart->setEnabled(true);
    });

    connect(ui->btn_audioSend, &QPushButton::pressed, this, [this]() {
        int     index    = currentAudioFile.lastIndexOf('/');
        QString fileName = currentAudioFile.mid(index + 1);

        opStatus = SEND_FILE;
        sendFlow->setFileName(fileName);
        sendFlow->setFileBlockSize(fileName, sysPara.blockSize);

        sendFlow->send(fileName, sysPara.blockIntervalTime, sysPara.repeatNum);
        opStatus = IDLE;
    });

    connect(ui->btn_audioPlay, &QPushButton::pressed, this, [this]() {
        player->play();
    });

    //********************视频相关操作*************************************
    connect(ui->btn_cameraOpen, &QPushButton::pressed, this, [this]() {
        camera->start();
        QList<QCameraViewfinderSettings> ViewSets = camera->supportedViewfinderSettings();
        int                              i        = 0;
        qDebug() << "viewfinderResolutions sizes.len = " << ViewSets.length();

        for(i = 0; i < ViewSets.length(); i++)
        {
            if(ViewSets[i].resolution() == QSize(640, 480))
                break;
        }
        camera->setViewfinderSettings(ViewSets[i--]);
    });

    connect(ui->btn_cameraClose, &QPushButton::pressed, this, [this]() {
        camera->stop();
    });

    connect(ui->btn_capturePic, &QPushButton::pressed, this, [this]() {
        QString path = QDir::currentPath() + "/cache/master/";
        QString name = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz");
        cameraImageCapture->capture(path + name);

        opStatus = SEND_FILE;
        sendFlow->setFileName(path + name);
        sendFlow->setFileBlockSize(name, sysPara.blockSize);

        sendFlow->send(name, sysPara.blockIntervalTime, sysPara.repeatNum);
        opStatus = IDLE;
    });

    connect(ui->btn_cameraOpenVideo, &QPushButton::pressed, this, [this]() {
        cameraTimer->start(sysPara.cycleIntervalTime);
    });

    connect(ui->btn_cameraCloseVideo, &QPushButton::pressed, this, [this]() {
        cameraTimer->stop();
    });

    connect(cameraTimer, &QTimer::timeout, this, [this]() {
        QString path = QDir::currentPath() + "/cache/master/";
        QString name = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz");
        cameraImageCapture->capture(path + name);

        qDebug() << "Record time of the capture picture. file size = " << QFileInfo(path + name).size();
        opStatus = SEND_FILE;
        sendFlow->setFileName(path + name);
        sendFlow->setFileBlockSize(name, sysPara.blockSize);
        sendFlow->send(name, sysPara.blockIntervalTime, sysPara.repeatNum);

        // sendFlow->setFileName("cache/master/test16k.bin");
        // sendFlow->setFileBlockSize("test16k.bin", sysPara.blockSize);
        // sendFlow->send("test16k.bin", sysPara.blockIntervalTime, sysPara.repeatNum);
        opStatus = IDLE;
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
        heartBeatCnt++;
        if(tcpStatus == 0x01)
        {
            if(ui->checkBox_heartBeat->isChecked() != true)
                return;
            if(opStatus != IDLE)
                return;

            data.append("Heart:");
            data.append(Common::int2ba(heartBeatCnt));
            dispatch->encode(UserProtocol::HEART_BEAT, data);
        }
    }
}
