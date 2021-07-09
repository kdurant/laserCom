#ifndef AUDIORECORD_H
#define AUDIORECORD_H

#include <QtCore>
#include <QAudioRecorder>
#include <QAudioProbe>
#include <QMessageBox>

class AudioRecord : public QObject
{
    Q_OBJECT
private:
    QAudioRecorder* recorder;  //音频录音
    QAudioProbe*    probe;     //探测器
public:
    AudioRecord() :
        recorder(new QAudioRecorder()),
        probe(new QAudioProbe())
    {
    }

    QString configSaveAudio(void);

public slots:
    void record(void);
    void stop(void);
};
#endif
