#include "recvFile.h"

bool RecvFile::processFileBlock(QByteArray &data)
{
}

/**
 * @brief RecvFile::getFileBlock
 * 从数据流中，根据帧头和桢尾标志，截取出一个完整的文件块
 */
void RecvFile::paserNewData(QByteArray &data)
{
    int headOffset = 0;
    int tailOffset = 0;

    headOffset = data.indexOf(frameHead);
    tailOffset = data.indexOf(frameHead);

    if(headOffset >= 0 && tailOffset >= 0)
    {
        state = IDLE;
        return;
    }

    if(state == IDLE)
    {
        if(headOffset >= 0)
        {
            fileBlockData.append(data.mid(headOffset));
            state = RECV_DATA;
        }
    }

    if(state == RECV_DATA)
    {
        if(tailOffset >= 0)
        {
            fileBlockData.append(data.mid(0, tailOffset + 8));
            state = IDLE;
        }
        else
        {
            fileBlockData.append(data);
        }
    }
}
