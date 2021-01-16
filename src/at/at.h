#ifndef __AT_H_
#define __AT_H_
#include <QtCore>

typedef struct _instruction_
{
    QString context;
    QString hint;
} AT_INST;

class AT
{
public:
    QVector<AT_INST> AT_query = {
        {"CGMR", "查询设备版本号"},
        {"CGSN", "查询设备SN号"},
    };

    QVector<AT_INST> AT_exe = {
        {"NSPA", "保存设备参数"},
    };
    QVector<AT_INST> AT_setup = {
        {"SDAC", "设置DAC默认高压"},
        {"PCKI", "设置发送数据包间隔(us)"},
    };
};

#endif
