#ifndef __TRANSFER_FILE_
#define __TRANSFER_FILE_

#include <QObject>
class TransferFile : public QObject
{
private:
public:
    TransferFile() = default;
    TransferFile(const TransferFile& cpy);
    TransferFile& operator=(const TransferFile& assign)
    {
        //this->m_a = as.m_a;
        return *this;
    };
    ~TransferFile();

    // 类内声明，类外定义，非成员函数，但可以直接访问类私有变量
    // friend void func(TransferFile & data);
};

#endif
