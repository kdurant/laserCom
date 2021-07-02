#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <QtCore>
/*

 */

#define FrameHead "0123456789abcdef"
#define FrameTail "1032547698badcfe"
class UserProtocol
{
public:
    enum MasterSet
    {
        SET_FILE_INFO    = 0x20,
        SET_FILE_DATA    = 0x30,
        SET_TEST_PATTERN = 0x40,
    };

    enum SlaveUp
    {
        HEART_BEAT         = 0x10,
        RESPONSE_FILE_INFO = 0x21,
        RESPONSE_FILE_DATA = 0x31,
    };

    enum DevAddr
    {
        MASTER_DEV = 0xa0,
        SLAVE_DEV  = 0x50,
    };

    enum misc
    {
        SEP_CHAR = '?',
    };

    enum status
    {
        SUCCESS = 0x11,
    };
};

#endif
