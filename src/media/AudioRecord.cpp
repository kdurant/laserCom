#include "AudioRecord.h"

int AudioRecord::configSaveAudio(void)
{
    if(recorder->audioInputs().length() <= 0)
        return -1;
    recorder->setAudioInput(recorder->audioInputs().first());  //设置录入设备

    QString path = QDir::currentPath();
    path += "/backup/";
    path += QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss") + ".wav";
    //    QString path = "abcd.wav";
    recorder->setOutputLocation(QUrl::fromLocalFile(path));  //设置输出文件

    QAudioEncoderSettings settings;                                         //音频编码设置
    settings.setCodec(recorder->supportedAudioCodecs().first());            //编码
    settings.setSampleRate(recorder->supportedAudioSampleRates().first());  //采样率
    settings.setBitRate(32000);                                             //比特率
    settings.setChannelCount(1);                                            //通道数
    settings.setQuality(QMultimedia::EncodingQuality(1));                   //品质
    recorder->setAudioSettings(settings);                                   //音频设置

    return 0;
}

void AudioRecord::record()
{
    recorder->record();
}

void AudioRecord::stop()
{
    recorder->stop();
}