#ifndef CPROCESSRAWEPIGRAPHDATA_H
#define CPROCESSRAWEPIGRAPHDATA_H

#include <QtCore>
#include <QObject>

#include "qepigdatafourieranalysislib_global.h"


typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;


#define PATH_MAX                           256

#define EV_IN_CHOPPER_CHAN					0
#define EV_IN_HSM_CHAN						1
#define EV_IN_DETECTOR_CHAN					2
#define EV_IN_MONITOR_CHAN					3
#define EV_IN_TURBINE_OPEN_CHAN				4
#define EV_IN_TURBINE_CLOSE_CHAN			5
#define EV_IN_CARRIAGE_EST_POS_CHAN			6
#define EV_IN_CARRIAGE_MEAS_POS_CHAN		7

#define SCAN_FILENAME						0
#define OSCIL_FILENAME						1
#define CHOP_SPE_FILENAME					2
#define HSM_SPE_FILENAME					3

 class QEPIGDATAFOURIERANALYSISLIBSHARED_EXPORT CProcessRawEpigraphData: public QObject
{
     Q_OBJECT
public:
    // конструктор класса
    //CProcessRawEpigraphData();
    explicit CProcessRawEpigraphData(QObject *parent);
    // деструктор
    virtual ~CProcessRawEpigraphData();
    // обрабатыват сырой файл и записыват извлеченную информацию
    int ProcessFile(wchar_t*);
    // обрабатывает все сырые данные в диретории
    int ProcessDirectory(wchar_t*);
    // устонавливает уровень выдачи информации
    void SetVerbose(WORD ver)
    {	m_Verbose = ver;	};
    // задает директорию, где будут лежатьобработанные файлы
    void SetDirectoryForProcessedFiles(wchar_t*);
    // function for defining of number of raw files
    WORD GetNumberOfRawFiles(){ return m_NumOfRawFiles;};
 public slots:
    void Stop();

signals:
    void ProcessedFile();
private:
    WORD m_AdcRate;
    DWORD m_DataStep;
    wchar_t m_ConstPartOfDataFile[PATH_MAX];
    static const WORD m_ChanNumInHSMPrdDestrib;
    double m_HalfWindInHSMFreqDestrib;//Hz
    static const WORD m_ChanNumInChopperPrdDestrib;
    double m_HalfWindInChopFreqDestrib;//Hz
    WORD m_ChopWindNum;
    WORD m_HsmFrequency;
    WORD m_Verbose;
    WORD m_NumOfRawFiles;
    bool m_StopFlag;

    void CulculateNumOfRawFiles(wchar_t*);
    // заполняет распределение, отправленное в последнем операнде
    void PutEventInDestrib(DWORD value,
                           DWORD valueIn,
                           DWORD valueFin,
                           const WORD channelNumber,
                           DWORD * destribArray);
    void PutEventInDestrib(double value,
                           double valueIn,
                           double valueFin,
                           const WORD channelNumber,
                           DWORD * destribArray,
                           double&);
    void PutEventInFreqDestrib(double,
                           double,
                           double,
                           const WORD,
                           DWORD*  );
    bool GetTamplateForProcessedFilePath(wchar_t*,wchar_t*);
    // возвращает в первую переменную путь к директории с сырими файлами и заканчивается слешом
    //второй операнд путь к сырому файлу
    bool GetDirectoryPath(wchar_t*,wchar_t*);
    //возвращает в массив пути к обаботанным файлам
    void GetProcessedFileName(WORD modFr,
            WORD hsmFr,
            WORD carriagePos,
            wchar_t [][PATH_MAX]);
    // создает общюю основу для пути к обработанным файлам
    void SetTamplateForProcessedFilePath(const wchar_t*);

    //удаляет все файлы из выбранной директории
    int DeleteFilesInDirectory(wchar_t*);
};

#endif // CPROCESSRAWEPIGRAPHDATA_H
