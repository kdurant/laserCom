#include "ProtocolDispatch.h"
#include <QMessageBox>

quint32 ProtocolDispatch::cmdNum = 0;

/**
 * @brief 根据协议预定好的命令，将收到的信息，发送给指定的模块处理
 * 简单的响应
 * 有两种情况：
 * 一、接收指令时，采用同步阻塞方式，TCP不会粘包. 将数据段内容  发送给指定的模块处理
 * 二、接收文件数据时，TCP肯定会粘包，如何将文件块分开，分别处理？
 * @param data
 */
void ProtocolDispatch::parserFrame(QByteArray &data)
{
    QByteArray head = Common::QString2QByteArray(FrameHead);
    QByteArray tail = Common::QString2QByteArray(FrameTail);
    QByteArray command;
    int        headOffset;
    int        tailOffset;

    frame.append(data);
start:
    headOffset = -1;
    tailOffset = -1;
    headOffset = frame.indexOf(head);
    tailOffset = frame.indexOf(tail);
    if(headOffset == -1 || tailOffset == -1)
    {
        // 一次TCP数据里没有帧头和帧尾, 等下一次进来数据再接着处理
        if(headOffset == -1 && tailOffset == -1)
        {
            return;
        }
        else if(headOffset == -1 && tailOffset > -1)
        {  // 没有帧头，但有帧尾，说明帧头数据丢失
            frame = frame.mid(tailOffset + 8);
        }
        else if(headOffset > -1 && tailOffset == -1)
        {  // 文件块数据的开始
            // 等下一次进来数据再接着处理
            return;
        }
    }
    else
    {
        command = frame.mid(headOffset, tailOffset + 8 - headOffset);
        processCommand(command);
        frame = frame.mid(tailOffset + 8);
        //        qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>>frame.size() = " << frame.size();
        if(frame.isEmpty() == false)
            goto start;
    }
}

void ProtocolDispatch::processCommand(QByteArray &frame)
{
    int        len        = frame.size();
    QByteArray validData  = frame.mid(0, len - 24);
    QByteArray recv_md5   = frame.mid(len - 24, 16);
    QByteArray expect_md5 = QCryptographicHash::hash(validData, QCryptographicHash::Md5);
    if(recv_md5 != expect_md5)
    {
        QString error = "Checksum Error!";
        emit    errorDataReady(error);
        return;
    }

    uint32_t   command  = getCommand(frame);
    uint32_t   data_len = getDataLen(frame);
    QByteArray transmitFrame;
    switch(command)
    {
        // 主机，从机进行相同的处理
        case UserProtocol::SlaveUp::HEART_BEAT:
            transmitFrame = frame.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
            transmitFrame = transmitFrame.mid(6, 4);
            emit heartBeatReady(Common::ba2int(transmitFrame));
            break;

        // 1. 主机发送SET_FILE_INFO
        // 2. 作为从机，收到主机发送的SET_FILE_INFO命令
        case UserProtocol::MasterSet::SET_FILE_INFO:
            transmitFrame = frame.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
            emit slaveFileInfoReady(transmitFrame);
            break;

        //3. 作为主机，收到从机发送的RESPONSE_FILE_INFO
        case UserProtocol::SlaveUp::RESPONSE_FILE_INFO:
            transmitFrame = frame.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
            emit masterFileInfoReady(transmitFrame);
            break;

        // 1. 主机发送SET_FILE_DATA
        // 2. 作为从机，收到主机发送的文件块数据
        case UserProtocol::MasterSet::SET_FILE_DATA:
            emit slaveFileBlockReady(frame);
            break;

        // 3. 作为主机，收到从机发送的RESPONSE_FILE_DATA
        case UserProtocol::SlaveUp::RESPONSE_FILE_DATA:
            transmitFrame = frame.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
            emit masterFileBlockReady(transmitFrame);
            break;

        default:
            QString error = "Undefined command received!";
            emit    errorDataReady(error);
            break;
    }
}

void ProtocolDispatch::encode(qint32 command, QByteArray &data)
{
    QByteArray frame;
    quint8     frameHead[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    frame.append((char *)frameHead, 8);

    QByteArray tmp;
    switch(command)
    {
        case UserProtocol::HEART_BEAT:
            frame.append(UserProtocol::SLAVE_DEV);   // source addr
            frame.append(UserProtocol::MASTER_DEV);  // destination addr
            frame.append(UserProtocol::HEART_BEAT);  // command
            tmp = Common::int2ba(data.size());
            frame.append(tmp);
            frame.append(data);
            break;

        case UserProtocol::SET_FILE_INFO:
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::SET_FILE_INFO);
            tmp = Common::int2ba(data.size());
            frame.append(tmp);
            frame.append(data);
            break;
        case UserProtocol::RESPONSE_FILE_INFO:
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::RESPONSE_FILE_INFO);
            tmp = Common::int2ba(data.size());
            frame.append(tmp);
            frame.append(data);
            break;

        case UserProtocol::SET_FILE_DATA:
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::SET_FILE_DATA);
            tmp = Common::int2ba(data.size());
            frame.append(tmp);
            frame.append(data);
            break;
        case UserProtocol::RESPONSE_FILE_DATA:
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::RESPONSE_FILE_DATA);
            tmp = Common::int2ba(data.size());
            frame.append(tmp);
            frame.append(data);
            break;
        default:
            break;
    }
    QByteArray md5 = QCryptographicHash::hash(frame, QCryptographicHash::Md5);
    frame.append(md5);

    quint8 frameTail[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};
    frame.append((char *)frameTail, 8);

    emit frameDataReady(frame);
}
