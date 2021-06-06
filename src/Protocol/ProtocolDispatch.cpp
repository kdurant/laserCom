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

    if(data.startsWith(head) && data.endsWith(tail))
    {
        int        len        = data.size();
        QByteArray validData  = data.mid(0, len - 24);
        QByteArray recv_md5   = data.mid(len - 24, 16);
        QByteArray expect_md5 = QCryptographicHash::hash(validData, QCryptographicHash::Md5);
        if(recv_md5 != expect_md5)
        {
            QString error = "Checksum Error!";
            emit    errorDataReady(error);
            return;
        }

        uint32_t   command  = getCommand(data);
        uint32_t   data_len = getDataLen(data);
        QByteArray transmitFrame;
        switch(command)
        {
            case UserProtocol::SlaveUp::HEART_BEAT:
                transmitFrame = data.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
                transmitFrame = transmitFrame.mid(6, 4);
                emit heartBeatReady(Common::ba2int(transmitFrame));
                break;
            case UserProtocol::SlaveUp::RESPONSE_FILE_INFO:
                transmitFrame = data.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);

                if(getMasterAddr(data) == UserProtocol::MASTER_DEV)  // 从机收到主机的信息数据
                    emit slaveFileInfoReady(transmitFrame);
                else  // 主机收到从机对文件信息的应答
                    emit masterFileInfoReady(transmitFrame);
                break;

            case UserProtocol::SlaveUp::RESPONSE_FILE_DATA:
                transmitFrame = data.mid(FrameField::DATA_POS + FrameField::DATA_LEN, data_len);
                emit masterFileBlockReady(transmitFrame);
                break;
            default:
                QString error = "Undefined command received!";
                emit    errorDataReady(error);
                break;
        }
    }
    else
    {
        emit slaveFileBlockReady(data);
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
