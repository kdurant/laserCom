#ifndef PROTOCOL_DISPATCH_H
#define PROTOCOL_DISPATCH_H

#include <QtCore>
#include "common.h"
#include "protocol.h"

class FrameField
{
public:
    enum fieldOffset
    {
        COMMAND_POS = 10,
        COMMAND_LEN = 1,
        DATA_POS    = 12,
        DATA_LEN    = 2
    };
};

/**
 * @brief 功能如下：
 * 1. 解析数据帧的命令字段内容，发射相应的信号给具体的处理模块
 * 2. 提供一个槽函数，需要按照协议打包数据的模块，可以通过信号将数据发送进来
 * 3. 数据打包后，发送信号，通知UDP将数据发送出去
 */
class ProtocolDispatch : public QObject
{
    Q_OBJECT
public:
    ProtocolDispatch()
    {
        //        cmdNum = 0;
    }
    ~ProtocolDispatch()
    {
    }

    void parserFrame(QByteArray &data);

    static uint32_t getCommand(QByteArray &data)
    {
        return data.mid(FrameField::COMMAND_POS, FrameField::COMMAND_LEN).toHex().toUInt(nullptr, 16);
    }
    static int getPckNum(QByteArray &data)
    {
        return data.mid(FrameField::PCK_NUM_POS, FrameField::PCK_NUM_LEN).toHex().toInt(nullptr, 16);
    }
    static int getDataLen(QByteArray &data)
    {
        return data.mid(FrameField::VALID_DATA_LEN_POS, FrameField::VALID_DATA_LEN_LEN).toHex().toInt(nullptr, 16);
    }

public slots:
    void encode(qint32 command, qint32 data_len, QByteArray &data);

signals:
    /**
     * @brief 系统状态不转发，直接返回
     * @param data
     */
    void infoDataReady(QByteArray &data);
    void heartBeatReady(quint32 number);

    void frameDataReady(QByteArray &data);

private:
    QVector<quint8>  head;
    static quint32   cmdNum;
    quint32          cmdData;
    quint32          packetNum{0};
    quint32          validDataLen;
    QVector<quint16> data;
    quint32          checksum;

    QString deviceVersion;
};
#endif
