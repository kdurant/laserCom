#include "sendFile.h"
void SendFile::splitData()
{
    QFile file(fileName);
    if(!file.exists())
        return;
    file.open(QIODevice::ReadOnly);
    file.readAll();
}
