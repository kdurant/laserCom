#ifndef SENDFILE_H
#define SENDFILE_H
#include <QtCore>
class SendFile
{
    QString             fileName;
    QVector<QByteArray> frame;  // 将文件按照协议分割保存，便与后面顺序发送

private:
public:
    SendFile() = default;
    void setFileName(QString const &name)
    {
        fileName = name;
    }

    void splitData(void);
};
#endif
