#include "ProtocolDispatch.h"
#include <QMessageBox>

quint32 ProtocolDispatch::cmdNum = 0;

/**
 * @brief 根据协议预定好的命令，将收到的信息，发送给指定的模块处理
 * @param data
 */
void ProtocolDispatch::parserFrame(QByteArray &data)
{
    uint32_t   command  = getCommand(data);
    uint32_t   data_len = getDataLen(data);
    QByteArray transmitFrame;
    switch(command)
    {
        case UserProtocol::SlaveUp::HEART_BEAT:
            transmitFrame = data.mid(FrameField::DATA_POS + 2, data_len);
            transmitFrame = transmitFrame.mid(5, 4);
            // emit heartBeatReady(transmitFrame);
            break;
        case UserProtocol::SlaveUp::RESPONSE_FILE_INFO:
            break;

        default:
            QString error = "Undefined command received!";
            //emit    errorDataReady(error);
            break;
    }
}

void ProtocolDispatch::encode(qint32 command, QByteArray &data)
{
    QByteArray frame;
    quint8     frameHead[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    frame.append((char *)frameHead, 8);
    switch(command)
    {
        case UserProtocol::HEART_BEAT:
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::HEART_BEAT);
            frame.append(data);
            break;

        case UserProtocol::SET_FILE_INFO:
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::SET_FILE_INFO);
            frame.append(data);
            break;
        case UserProtocol::RESPONSE_FILE_INFO:
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::RESPONSE_FILE_INFO);
            frame.append(data);
            break;

        case UserProtocol::SET_FILE_DATA:
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::SET_FILE_DATA);
            frame.append(data);
            break;
        case UserProtocol::RESPONSE_FILE_DATA:
            frame.append(UserProtocol::SLAVE_DEV);
            frame.append(UserProtocol::MASTER_DEV);
            frame.append(UserProtocol::RESPONSE_FILE_DATA);
            frame.append(data);
            break;
        default:
            break;
    }
    quint8 frameHead[] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};
    frame.append((char *)frameHead, 8);

    QByteArray md5 = QCryptographicHash::hash(frame, QCryptographicHash::Md5);
    frame.append(md5);
    emit frameDataReady(frame);
}
