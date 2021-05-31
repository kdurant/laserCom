#ifndef PROTOCOL_H
#define PROTOCOL_H

/*

 */

class MasterSet
{
public:
    enum master
    {
        SET_FILE_INFO = 0x20,
        SET_FILE_DATA = 0x30,
    }
};

class SlaveUp
{
public:
    enum slave
    {
        HEART_BEAT         = 0x10,
        RESPONSE_FILE_INFO = 0x21,
        RESPONSE_FILE_DATA = 0x31,
    };
};

#endif
