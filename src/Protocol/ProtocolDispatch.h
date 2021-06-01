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
        DATA_POS    = 11,
        DATA_LEN    = 4
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

    static uint32_t getDataLen(QByteArray &data)
    {
        return data.mid(FrameField::DATA_POS, FrameField::DATA_LEN).toHex().toUInt(nullptr, 16);
    }
public slots:
    void encode(qint32 command, QByteArray &data);

signals:
    /**
     * @brief 系统状态不转发，直接返回
     * @param data
     */
    void heartBeatReady(quint32 number);

    void fileInfoReady(QByteArray &data);
    void fileBlockReady(QByteArray &data);
    void frameDataReady(QByteArray &data);

    void errorDataReady(QString &data);

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
