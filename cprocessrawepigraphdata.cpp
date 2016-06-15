#include "cprocessrawepigraphdata.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <wchar.h>
#include <cstring>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdexcept>
#include <sys/types.h>
#include <dirent.h>


using namespace std;



//namespace ProcessRawFiles
//{
    const WORD CProcessRawEpigraphData::m_ChanNumInHSMPrdDestrib = 100;
    const WORD CProcessRawEpigraphData::m_ChanNumInChopperPrdDestrib = 100;
    //wchar_t m_ConstPartOfDataFile[PATH_MAX] = {NULL};
    CProcessRawEpigraphData::CProcessRawEpigraphData()
    {
        m_AdcRate = 5000;
        m_DataStep = 1024*1024;
        m_HalfWindInHSMFreqDestrib = 18.;//Hz
        m_HalfWindInChopFreqDestrib = 0.08;//Hz
        m_ChopWindNum = 12;
        m_HsmFrequency  = 25;
        m_Verbose = 0;
    }
    CProcessRawEpigraphData::~CProcessRawEpigraphData()
    {

    }
    int CProcessRawEpigraphData::ProcessFile(wchar_t* rawDataFilePath)
    {
        if(wcslen(m_ConstPartOfDataFile) == 0)
            if(!GetTamplateForProcessedFilePath(m_ConstPartOfDataFile,rawDataFilePath))
                wcscpy(m_ConstPartOfDataFile,L"~/default");
        ifstream inClientFile;
        char filePathBuffer[PATH_MAX] = {NULL};
        wcstombs(filePathBuffer,rawDataFilePath,sizeof(filePathBuffer));
        inClientFile.open(filePathBuffer,ios::in | ios::binary);
        if(!inClientFile) return 43;
        BYTE eventID(0);
        long long time[2] = {NULL};
        // времена прихода и ухода турбины
        DWORD turbineOpenTime[3] = {NULL};
        WORD turbineOpenCounter(0);
        DWORD turbineCloseTime[2] = {NULL};
        WORD turbineCloseCounter(0);
        // c учетом максимальной загрузки детестора 300Гц
        // и временем измерия в фале 20с заведем массив на 7000 эл
        DWORD detectorPulseTime[7000] = {NULL};
        WORD detectorPulseCounter(0);
        // счетчик нейтронов в период, когда турбинв открыта
        DWORD detectorPulseInTurbOpenCounter(0);
        // монитор макс загрузка монитора 10 раз больше
        DWORD monitorPulseTime[70000] = {NULL};
        WORD monitorPulseCounter(0);
        // время прихода импулься с прерывателя
        // макс c запасом чатоста 150гц
        DWORD modulatorPulseTime[3000] = {NULL};
        WORD modulatorPulseCounter(0);
        // время прихода импульса с решетки
        DWORD hsmMotorPulseTime[4000] = {NULL};
        WORD hsmMotorPulseCounter(0);
        // положение каретки
        WORD estCarriagePos(0);
        double measCarriagePos(0.);
        // массив для построения распределения времени пролета на
        //переоде осциляции с количеством каналов
        const WORD oscilChanelNumber(12*12);
        DWORD oscilDestrib[oscilChanelNumber] = {NULL};


        // время измерения мезду двумя крайними импульсами преррывателя
        // лежащих внутри ортезка времени когда турбина открыта
        // в пределах обрабатываемого файла
        DWORD measModTimeInFileInADCTicks(0);


        // время измерения в файле в тиках
        DWORD measTimeInFileInADCTicks(0);
        //DWORD adcRate(5000), dataStep(1024*1024);
        WORD error(0);
        // первая запись должна быть состояние турбины
        if(inClientFile>>eventID &&
           inClientFile.read(reinterpret_cast<char *>(&time[0]),5) &&
           !inClientFile.eof())
        {
            switch(eventID)
            {
            case EV_IN_TURBINE_OPEN_CHAN:
                turbineOpenTime[turbineOpenCounter++] = 0;
                break;
            case EV_IN_TURBINE_CLOSE_CHAN:
                turbineCloseTime[turbineCloseCounter++] = 0;
                break;
            default:
                // закроем файл
                inClientFile.close();
                return 44;
            }
        }
        else
        {
            // закроем файл
            inClientFile.close();
            return 45;
        }
        // две вторые записи должны быть положение каретки выставленное

        if(inClientFile>>eventID &&
            inClientFile.read(reinterpret_cast<char *>(&time[1]),5) &&
            !inClientFile.eof())
        {
            if(eventID == EV_IN_CARRIAGE_EST_POS_CHAN) estCarriagePos = time[1];
            else
            {
                // закроем файл
                inClientFile.close();
                return 44;
            }
        }
        // и измеренное линейкой
        if(inClientFile>>eventID && !inClientFile.eof())
        {
            if(eventID == EV_IN_CARRIAGE_MEAS_POS_CHAN)
            {
                if(!inClientFile.read(reinterpret_cast<char *>(&measCarriagePos),5) &&
                        inClientFile.eof())
                {
                    // закроем файл
                    inClientFile.close();
                    return 44;
                }
            }
            else
            {
                // закроем файл
                inClientFile.close();
                return 44;
            }
        }
        // теперь приступем к обработке всех каналов АЦП
        // и рассортируем признаки
        while(inClientFile>>eventID &&
            inClientFile.read(reinterpret_cast<char *>(&time[1]),5) &&
            !inClientFile.eof())
        {
            switch(eventID)
            {
            case EV_IN_CHOPPER_CHAN:
                modulatorPulseTime[modulatorPulseCounter++] = DWORD(time[1]-time[0]);
                break;
            case EV_IN_HSM_CHAN:
                hsmMotorPulseTime[hsmMotorPulseCounter++] = DWORD(time[1]-time[0]);
                break;
            case EV_IN_MONITOR_CHAN:
                monitorPulseTime[monitorPulseCounter++] = DWORD(time[1]-time[0]);
                break;
            case EV_IN_DETECTOR_CHAN:
                detectorPulseTime[detectorPulseCounter++] = DWORD(time[1]-time[0]);
                break;
            case EV_IN_TURBINE_OPEN_CHAN:
                turbineOpenTime[turbineOpenCounter++] = DWORD(time[1]-time[0]);
                break;
            case EV_IN_TURBINE_CLOSE_CHAN:
                turbineCloseTime[turbineCloseCounter++] = DWORD(time[1]-time[0]);
                break;
            default:
                // закроем файл
                inClientFile.close();
                return 44;
            }
        }
        // закроем файл, если он был открыт для считывания сырых данных
        if(inClientFile.is_open()) inClientFile.close();
        // проверим есть ли импульсы в канале детектора, если нет выходим
        if(detectorPulseCounter == 0)
        {
            // закроем файл
            inClientFile.close();
            return 46;
        }
        // здесь начнется обрадотка данных загруженных из файла
        // посмотрим есть ли импульсы в канале hsm
        // и будем записывать распределение частоты
        DWORD HalfWindTimeInHSMPrdDestrib = 0; //usec
        DWORD HalfWindTimeInChopPrdDestrib = 0; //usec
        WORD modFrequency(0);
        double rotFrequency(0.);
        //m_HalfWindInChopFreqDestrib /= m_ChopWindNum;
        DWORD hsmPeriodDistr[m_ChanNumInHSMPrdDestrib] = {NULL};
        DWORD chopPeriodDistr[m_ChanNumInChopperPrdDestrib] = {NULL};
        if(hsmMotorPulseCounter > 1)
        {
            DWORD sum(0),inOfPrdDistr(0),finOfPrdDistr(0),avrPrd(0); // all in ticks
            //m_HsmFrequency = WORD(m_AdcRate*1000.*(hsmMotorPulseCounter-1)/sum+0.5);
            avrPrd = DWORD(1000.*m_AdcRate/m_HsmFrequency);
            HalfWindTimeInHSMPrdDestrib = avrPrd -
                    DWORD(1000.*m_AdcRate/(1.*m_HsmFrequency + m_HalfWindInHSMFreqDestrib));
            inOfPrdDistr = avrPrd - HalfWindTimeInHSMPrdDestrib;
            finOfPrdDistr = avrPrd + HalfWindTimeInHSMPrdDestrib;
            for(WORD i(1);i<hsmMotorPulseCounter;i++)
                PutEventInDestrib(hsmMotorPulseTime[i] - hsmMotorPulseTime[i-1],
                                    inOfPrdDistr,
                                    finOfPrdDistr,
                                    m_ChanNumInHSMPrdDestrib,
                                    hsmPeriodDistr);
        }
        // посмотрим есть ли импульсы в канале чоппера
        // и будем записывать распределение частоты
        if(modulatorPulseCounter > 1)
        {
            //cout<<"here"<<endl;
            DWORD sum(0),inOfPrdDistr(0),finOfPrdDistr(0),avrPrd(0); // all in ticks
            sum = modulatorPulseTime[modulatorPulseCounter-1] - modulatorPulseTime[0];
            //cout<<modFrequency<<" "<<sum<<" "<<modulatorPulseCounter<<endl;
            modFrequency = WORD(1000.*m_ChopWindNum*m_AdcRate*(modulatorPulseCounter-1)/sum+0.5);
            rotFrequency = 1.*modFrequency/m_ChopWindNum;
            avrPrd = DWORD(1000.*m_AdcRate/rotFrequency);
            HalfWindTimeInChopPrdDestrib = avrPrd -
                                           DWORD(1000.*m_AdcRate/(rotFrequency + m_HalfWindInChopFreqDestrib));
            inOfPrdDistr = avrPrd - HalfWindTimeInChopPrdDestrib;
            finOfPrdDistr = avrPrd + HalfWindTimeInChopPrdDestrib;
            for(WORD i(1);i<modulatorPulseCounter;i++)
                PutEventInDestrib(modulatorPulseTime[i] - modulatorPulseTime[i-1],
                                    inOfPrdDistr,
                                    finOfPrdDistr,
                                    m_ChanNumInChopperPrdDestrib,
                                    chopPeriodDistr);
        }
        // помотрим есть ли импульсы в каналах прерывателя
        // и заполним массив распределения
        // проверим есть ли интервалы когда турбина у нас
        switch (turbineOpenCounter)
        {
        // файл содержит данный, полученные когда турбина была не у нас
        // эти данные нам не нужны
        case 0:
            // закроем файл
            inClientFile.close();
            return 0;
        case 1:
        {
            // найдем первый и последний импульс от прерывателя лежащий в период открытой турбины
            // и заодно вычеслим частоту модуляции
            //open-closed
            int firstModPulseInOpenTurb(-1), lastModPulseInOpenTurb(-1);
            if(modulatorPulseCounter > 1)
            {
                for(WORD i(1);i<modulatorPulseCounter;i++)
                {
                    // найдем первый импульс
                    if(turbineOpenTime[0]<modulatorPulseTime[i-1] && firstModPulseInOpenTurb == -1)
                        firstModPulseInOpenTurb = i-1;
                    // найдем последний импульс
                    if(turbineCloseTime[0]-m_DataStep > modulatorPulseTime[modulatorPulseCounter-i] &&
                       lastModPulseInOpenTurb == -1)
                        lastModPulseInOpenTurb = modulatorPulseCounter-i;
                    if(firstModPulseInOpenTurb != -1 && lastModPulseInOpenTurb != -1) break;
                }
                measModTimeInFileInADCTicks = modulatorPulseTime[lastModPulseInOpenTurb] -
                                                  modulatorPulseTime[firstModPulseInOpenTurb];
            }

            for(WORD i(0);i < detectorPulseCounter;i++)
            {
                if(turbineOpenTime[0] <= detectorPulseTime[i] &&
                    detectorPulseTime[i] <= turbineCloseTime[0]-m_DataStep)
                {
                    detectorPulseInTurbOpenCounter++;
                    if(firstModPulseInOpenTurb != lastModPulseInOpenTurb)
                    {
                        if(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb])
                        {
                            while(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb+1] &&
                                  firstModPulseInOpenTurb+1 != lastModPulseInOpenTurb)
                                firstModPulseInOpenTurb++;
                            if(detectorPulseTime[i] < modulatorPulseTime[lastModPulseInOpenTurb])
                                // заполним распределение
                                PutEventInDestrib(detectorPulseTime[i],
                                                    modulatorPulseTime[firstModPulseInOpenTurb],
                                                    modulatorPulseTime[firstModPulseInOpenTurb+1],
                                                    oscilChanelNumber,
                                                    oscilDestrib);
                        }
                    }
                }
            }
            measTimeInFileInADCTicks = turbineCloseTime[0]-m_DataStep - turbineOpenTime[0];
            break;
        }
        case 2:
        {
            // найдем первый и последний импульс от прерывателя лежащий в период открытой турбины
            // и заодно вычеслим сатоту модуляции
             //open-open
            int firstModPulseInOpenTurb(-1), lastModPulseInOpenTurb(-1);
            if(modulatorPulseCounter > 1)
            {
                for(WORD i(1);i<modulatorPulseCounter;i++)
                {
                    // найдем первый импульс
                    if(turbineOpenTime[0]<modulatorPulseTime[i-1] && firstModPulseInOpenTurb == -1)
                        firstModPulseInOpenTurb = i-1;
                    // найдем последний импульс
                    if(turbineOpenTime[1] > modulatorPulseTime[modulatorPulseCounter-i] && lastModPulseInOpenTurb == -1)
                        lastModPulseInOpenTurb = modulatorPulseCounter-i;
                    if(firstModPulseInOpenTurb != -1 && lastModPulseInOpenTurb != -1) break;
                }
                measModTimeInFileInADCTicks = modulatorPulseTime[lastModPulseInOpenTurb] -
                                                  modulatorPulseTime[firstModPulseInOpenTurb];
            }
            // посчитем количество нейтронов когда турбина открыта
            for(WORD i(0);i < detectorPulseCounter;i++)
            {
                if(turbineOpenTime[0] <= detectorPulseTime[i] &&
                    detectorPulseTime[i] <= turbineOpenTime[1])
                {
                    detectorPulseInTurbOpenCounter++;
                    if(firstModPulseInOpenTurb != lastModPulseInOpenTurb)
                    {
                        if(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb])
                        {
                            while(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb+1] &&
                                  firstModPulseInOpenTurb+1 != lastModPulseInOpenTurb)
                                firstModPulseInOpenTurb++;
                            if(detectorPulseTime[i] < modulatorPulseTime[lastModPulseInOpenTurb])
                                // заполним распределение
                                PutEventInDestrib(detectorPulseTime[i],
                                                    modulatorPulseTime[firstModPulseInOpenTurb],
                                                    modulatorPulseTime[firstModPulseInOpenTurb+1],
                                                    oscilChanelNumber,
                                                    oscilDestrib);
                        }
                    }
                }

            }
            measTimeInFileInADCTicks = turbineOpenTime[1]-turbineOpenTime[0];
            break;
        }
        case 3:
        {
            // it setuation when in file turb open-close-open-open
            // найдем первый и последний импульс от прерывателя лежащий в период открытой турбины
            // и заодно вычеслим частоту модуляции

            //open-close
            int firstModPulseInOpenTurb(-1), lastModPulseInOpenTurb(-1);
            if(modulatorPulseCounter != 0)
            {
                for(WORD i(1);i<modulatorPulseCounter;i++)
                {
                    // найдем первый импульс
                    if(turbineOpenTime[0]<modulatorPulseTime[i-1] && firstModPulseInOpenTurb == -1)
                        firstModPulseInOpenTurb = i-1;
                    // найдем последний импульс
                    if(turbineCloseTime[0]-m_DataStep > modulatorPulseTime[modulatorPulseCounter-i] &&
                        lastModPulseInOpenTurb == -1)
                        lastModPulseInOpenTurb = modulatorPulseCounter-i;
                    if(firstModPulseInOpenTurb != -1 && lastModPulseInOpenTurb != -1) break;
                }
                measModTimeInFileInADCTicks = modulatorPulseTime[lastModPulseInOpenTurb] -
                    modulatorPulseTime[firstModPulseInOpenTurb];
            }

            for(WORD i(0);i < detectorPulseCounter;i++)
            {
                if(turbineOpenTime[0] <= detectorPulseTime[i] &&
                    detectorPulseTime[i] <= turbineCloseTime[0]-m_DataStep)
                {
                    detectorPulseInTurbOpenCounter++;
                    if(firstModPulseInOpenTurb != lastModPulseInOpenTurb)
                    {
                        if(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb])
                        {
                            while(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb+1] &&
                                firstModPulseInOpenTurb+1 != lastModPulseInOpenTurb)
                                firstModPulseInOpenTurb++;
                            if(detectorPulseTime[i] < modulatorPulseTime[lastModPulseInOpenTurb])
                                // заполним распределение
                                PutEventInDestrib(detectorPulseTime[i],
                                modulatorPulseTime[firstModPulseInOpenTurb],
                                modulatorPulseTime[firstModPulseInOpenTurb+1],
                                oscilChanelNumber,
                                oscilDestrib);
                        }
                    }
                }
            }
            measTimeInFileInADCTicks = turbineCloseTime[0] - m_DataStep - turbineOpenTime[0];
            //open-open
            firstModPulseInOpenTurb = -1;
            lastModPulseInOpenTurb = -1;
            if(modulatorPulseCounter > 1)
            {
                for(WORD i(1);i<modulatorPulseCounter;i++)
                {
                    // найдем первый импульс
                    if(turbineOpenTime[1]<modulatorPulseTime[i-1] && firstModPulseInOpenTurb == -1)
                        firstModPulseInOpenTurb = i-1;
                    // найдем последний импульс
                    if(turbineOpenTime[2] > modulatorPulseTime[modulatorPulseCounter-i] && lastModPulseInOpenTurb == -1)
                        lastModPulseInOpenTurb = modulatorPulseCounter-i;
                    if(firstModPulseInOpenTurb != -1 && lastModPulseInOpenTurb != -1) break;
                }
                measModTimeInFileInADCTicks += (modulatorPulseTime[lastModPulseInOpenTurb] -
                    modulatorPulseTime[firstModPulseInOpenTurb]);
            }
            // посчитем количество нейтронов когда турбина открыта
            for(WORD i(0);i < detectorPulseCounter;i++)
            {
                if(turbineOpenTime[1] <= detectorPulseTime[i] &&
                    detectorPulseTime[i] <= turbineOpenTime[2])
                {
                    detectorPulseInTurbOpenCounter++;
                    if(firstModPulseInOpenTurb != lastModPulseInOpenTurb)
                    {
                        if(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb])
                        {
                            while(detectorPulseTime[i] > modulatorPulseTime[firstModPulseInOpenTurb+1] &&
                                firstModPulseInOpenTurb+1 != lastModPulseInOpenTurb)
                                firstModPulseInOpenTurb++;
                            if(detectorPulseTime[i] < modulatorPulseTime[lastModPulseInOpenTurb])
                                // заполним распределение
                                PutEventInDestrib(detectorPulseTime[i],
                                modulatorPulseTime[firstModPulseInOpenTurb],
                                modulatorPulseTime[firstModPulseInOpenTurb+1],
                                oscilChanelNumber,
                                oscilDestrib);
                        }
                    }
                }

            }
            measTimeInFileInADCTicks += (turbineOpenTime[2]-turbineOpenTime[1]);
            break;
        }
        default:
            // закроем файл
            inClientFile.close();
            return 44;
        }

        // инициалезуем переременные для записи обработтаного файла
        WORD pointCounter(0);
        WORD point[200] = {NULL};
        // количество событий в точке
        DWORD eventNumber[200]= {NULL};
        // время набора в точке в секундах
        WORD collectionTimeInSec[200] = {NULL},inThisFileCollTimeInSec( measTimeInFileInADCTicks/m_AdcRate/1000 );
        // остаток от времени набора в тиках АЦП
        DWORD balanceOfCollTimeInADCTicks[200] = {NULL},inThisFileBalanceOfCollTimeInADCTicks( measTimeInFileInADCTicks - inThisFileCollTimeInSec*m_AdcRate*1000 );
        // определим название обработанного файла
        wchar_t processedFileName[4][PATH_MAX] = {NULL};

        GetProcessedFileName(modFrequency,m_HsmFrequency,estCarriagePos, processedFileName);
    //	for(WORD i(0);i<4;i++)
    //		wcout<<processedFileName[i]<<endl;
        // считаем данные из ранее созданного файла спектра периодов hsm
        if(wcslen(processedFileName[HSM_SPE_FILENAME])>0)
        {
            WORD chan(0);
            DWORD val(0),nPrdFromFile(0);
            wcstombs(filePathBuffer,processedFileName[HSM_SPE_FILENAME],sizeof(filePathBuffer));
            inClientFile.open(filePathBuffer,ios::in | ios::out);
            if(inClientFile)
            {
                double freq(0.);
                while(inClientFile>>freq>>val && !inClientFile.eof())
                {
                    hsmPeriodDistr[chan++]+=val;
                    if(chan == m_ChanNumInHSMPrdDestrib)
                        break;
                }
                inClientFile>>nPrdFromFile;
            }
            if(inClientFile.is_open()) inClientFile.close();
            // сбросим данные распределения в частотах в файл
            ofstream outClientFile(filePathBuffer);
            DWORD prdStep(0),prdIn(0);// usec
            double freq(0.);
            prdStep = DWORD(2*HalfWindTimeInHSMPrdDestrib/m_ChanNumInHSMPrdDestrib);
            prdIn = DWORD(1000.*m_AdcRate/m_HsmFrequency) + HalfWindTimeInHSMPrdDestrib;
            for(WORD i(0);i<m_ChanNumInHSMPrdDestrib;i++)
            {
                freq = 1000.*m_AdcRate/(prdIn - prdStep*i);
                outClientFile<<freq<<" "<<hsmPeriodDistr[m_ChanNumInHSMPrdDestrib-1-i]<<"\n";
            }
            outClientFile<<(nPrdFromFile+hsmMotorPulseCounter-1)<<"\n";
        }
        // считаем данные из ранее созданного файла спектра периодов чоппера
        if(wcslen(processedFileName[CHOP_SPE_FILENAME])>0)
        {
            WORD chan(0);
            DWORD val(0),nPrdFromFile(0);
            wcstombs(filePathBuffer,processedFileName[CHOP_SPE_FILENAME],sizeof(filePathBuffer));
            inClientFile.open(filePathBuffer,ios::in | ios::out);
            if(inClientFile)
            {
                double freq(0.);
                while(inClientFile>>freq>>val && !inClientFile.eof())
                {
                    chopPeriodDistr[chan++]+=val;
                    if(chan == m_ChanNumInChopperPrdDestrib)
                        break;
                }
                inClientFile>>nPrdFromFile;
            }
            if(inClientFile.is_open()) inClientFile.close();
            // сбросим данные распределения в частотах в файл
            ofstream outClientFile(filePathBuffer);
            DWORD prdStep(0),prdIn(0);// usec
            double freq(0.);
            prdStep = DWORD(2*HalfWindTimeInChopPrdDestrib/m_ChanNumInChopperPrdDestrib);
            prdIn = DWORD(1000.*m_AdcRate/rotFrequency) + HalfWindTimeInChopPrdDestrib;
            for(WORD i(0);i<m_ChanNumInChopperPrdDestrib;i++)
            {
                freq = 1000.*m_AdcRate*m_ChopWindNum/(prdIn - prdStep*i);
                outClientFile<<freq<<" "<<chopPeriodDistr[m_ChanNumInChopperPrdDestrib-1-i]<<"\n";
            }
            outClientFile<<nPrdFromFile+modulatorPulseCounter-1<<" "<<rotFrequency<<"\n";
        }
        // считаем данные из ранее созданного файла спектра времение пролета
        if(wcslen(processedFileName[OSCIL_FILENAME])>0)
        {
            WORD chan(0);
            DWORD val(0);
            wcstombs(filePathBuffer,processedFileName[OSCIL_FILENAME],sizeof(filePathBuffer));
            inClientFile.open(filePathBuffer,ios::in | ios::out);
            if(inClientFile)
            {
                while(inClientFile>>chan>>val && !inClientFile.eof())
                {
                    oscilDestrib[chan]+=val;
                    if(chan == oscilChanelNumber-1)
                    {
                        inClientFile>>chan>>val;
                        break;
                    }
                }
            }
            if(inClientFile.is_open()) inClientFile.close();
            // сбросим данные распределения в файл
            ofstream outClientFile(filePathBuffer);
            for(WORD i(0);i<oscilChanelNumber;i++)
                outClientFile<<i<<" "<<oscilDestrib[i]<<"\n";
            WORD measTimeInSec = measModTimeInFileInADCTicks/m_AdcRate/1000;
            DWORD balanceOfMeasTimeInTicks(measModTimeInFileInADCTicks-measTimeInSec*m_AdcRate*1000);
            measTimeInSec += chan;
            balanceOfMeasTimeInTicks += val;
            if(balanceOfMeasTimeInTicks>m_AdcRate*1000)
            {
                measTimeInSec++;
                balanceOfMeasTimeInTicks -= m_AdcRate*1000;
            }
            outClientFile<<measTimeInSec<<" "<<balanceOfMeasTimeInTicks<<" "<<m_ChopWindNum<<"\n";
            outClientFile.close();
            // запишем файл для рисования графика
            //WriteGraphFile(modFrequency,oscilChanelNumber,oscilDestrib,measTimeInSec);
            // здесь надо обрабатывать распределение для фурье нужен файл с коэфициентами Фурье
            // после каждой частоты мы паправим нужные коэффициенты и создадим графический файл.
            // нужен еще признак что мы будем использовать постоение фурье, по нему будут предприниматься
            // все действия по построению фурье спектра
    //		if (CSpectrDlg::IsUsingFourier())
    //		{
    //			//извлечем данные из файла распределения
    //			CFourierDataExtract::ExtractDataFromFile(processedFileName[OSCIL_FILENAME].GetfilePathBuffer());
    //			//запишем файл графика с фурье распределением
    //			CFourierDataExtract::WriteFourierGraphFile(m_DataDirectory.GetBuffer(),0.1,0.2,200);
    //		}
        }
         //Заполним файлы для скана
        // считаем данные из ранее созданного файла для скана
        wcstombs(filePathBuffer,processedFileName[SCAN_FILENAME],sizeof(filePathBuffer));
        inClientFile.open(filePathBuffer,ios::in);
        if(inClientFile)
        {
            while(inClientFile>>point[pointCounter] && !inClientFile.eof())
            {
                inClientFile>>eventNumber[pointCounter]
                >>collectionTimeInSec[pointCounter]
                >>balanceOfCollTimeInADCTicks[pointCounter];
                pointCounter++;
            }
        }
        // закроем файл
        inClientFile.close();
        //запишем файл
        wcstombs(filePathBuffer,processedFileName[SCAN_FILENAME],sizeof(filePathBuffer));
        ofstream outClientFile(filePathBuffer,ios::out);
        if(!outClientFile) return 47;
        for(WORD n(0);n<pointCounter;n++)
        {
            if(estCarriagePos < point[n])
            {
                outClientFile<<estCarriagePos<<" "<<detectorPulseInTurbOpenCounter						<<" "<<inThisFileCollTimeInSec<<" "<<inThisFileBalanceOfCollTimeInADCTicks<<"\n";
                // допишем оставшиеся данные и выдем и программы
                for(WORD i(n);i<pointCounter;i++)
                {
                    outClientFile<<point[i]<<" "<<eventNumber[i]
                                <<" "<<collectionTimeInSec[i]<<" "<<balanceOfCollTimeInADCTicks[i]<<"\n";
                     // перезапишем массивы с учетом новоой точки
                     point[i] = estCarriagePos;
                     eventNumber[i] = detectorPulseInTurbOpenCounter;
                     collectionTimeInSec[i] = inThisFileCollTimeInSec;
                     if(i == pointCounter-1)
                     {
                         point[i+1] = estCarriagePos;
                         eventNumber[i+1] = detectorPulseInTurbOpenCounter;
                         collectionTimeInSec[i+1] = inThisFileCollTimeInSec;
                         pointCounter++;
                         break;
                     }
                     else
                     {
                         estCarriagePos = point[i+1];
                         detectorPulseInTurbOpenCounter = eventNumber[i+1];
                         inThisFileCollTimeInSec = collectionTimeInSec[i+1];
                     }
                }
    //			//запишем файл для графика
    //			if(!WriteGraphFile(pointCounter,point,eventNumber,collectionTimeInSec))return 48;
    //			// закроем файл
    //			outClientFile.close();
                return WORD(0);
            }
            else
            {
                if(estCarriagePos == point[n])
                {
                    eventNumber[n] += detectorPulseInTurbOpenCounter;
                    DWORD sum(balanceOfCollTimeInADCTicks[n]+inThisFileBalanceOfCollTimeInADCTicks);
                    if(sum > m_AdcRate*1000)
                    {
                        balanceOfCollTimeInADCTicks[n] = sum - m_AdcRate*1000;
                        collectionTimeInSec[n] += (++inThisFileCollTimeInSec);
                    }
                    else
                    {
                        collectionTimeInSec[n] +=  inThisFileCollTimeInSec;
                        balanceOfCollTimeInADCTicks[n] = sum;
                    }
                    outClientFile<<point[n]<<" "<<eventNumber[n]
                        <<" "<<collectionTimeInSec[n]<<" "<<balanceOfCollTimeInADCTicks[n]<<"\n";
                    // допишем оставшиеся данные и выдем и программы
                    for(WORD i(++n);i<pointCounter;i++)
                    {
                        outClientFile<<point[i]<<" "<<eventNumber[i]
                                     <<" "<<collectionTimeInSec[i]<<" "<<balanceOfCollTimeInADCTicks[i]<<"\n";
                    }
                    //запишем файл для графика
                    //if(!WriteGraphFile(pointCounter,point,eventNumber,collectionTimeInSec))return 48;
                    // закроем файл
                    //outClientFile.close();
                    return WORD(0);
                }
                else
                {
                    outClientFile<<point[n]<<" "<<eventNumber[n]
                                <<" "<<collectionTimeInSec[n]<<" "<<balanceOfCollTimeInADCTicks[n]<<"\n";
                }
            }
        }
        outClientFile<<estCarriagePos<<" "<<detectorPulseInTurbOpenCounter
                    <<" "<<inThisFileCollTimeInSec<<" "<<inThisFileBalanceOfCollTimeInADCTicks;
        // перезапишем массивы с учетом новоой точки
        point[pointCounter] = estCarriagePos;
        eventNumber[pointCounter] = detectorPulseInTurbOpenCounter;
        collectionTimeInSec[pointCounter] = inThisFileCollTimeInSec;
        pointCounter++;
        //запишем файл для графика
    //	if(!WriteGraphFile(pointCounter,point,eventNumber,collectionTimeInSec))return 48;
    //	outClientFile.close();
        return 0;
    }
    //------------------------------------------------
    void CProcessRawEpigraphData::PutEventInDestrib(DWORD value,
            DWORD valueIn,
            DWORD valueFin,
            const WORD channelNumber,
            DWORD * destribArray)
    {
        WORD step = (valueFin-valueIn)/channelNumber;
        for(WORD i(0);i<channelNumber;i++)
        {
            if((valueIn+i*step) < value &&
                value < (valueIn+(i+1)*step))
            {
                destribArray[i]++;
                return;
            }
        }
    }
    //-----------------------------------------------
    void CProcessRawEpigraphData::GetProcessedFileName(WORD modFr,
            WORD hsmFr,
            WORD carriagePos,
            wchar_t processedFileName[][PATH_MAX])
    {
        // определим названия файла для скана
        if(hsmFr == 0)
        {
            wcscpy(processedFileName[SCAN_FILENAME],m_ConstPartOfDataFile);
            wcscat(processedFileName[SCAN_FILENAME],L"_Scan.txt");
        }
        else
        {
            wcscpy(processedFileName[SCAN_FILENAME],m_ConstPartOfDataFile);
            wchar_t buffer [100];
            swprintf ( buffer, 100, L"_Scan_hsm%i.txt",hsmFr);
            wcsncat(processedFileName[SCAN_FILENAME],buffer,100);
            //файл для спектра частот
            wcscpy(processedFileName[HSM_SPE_FILENAME],m_ConstPartOfDataFile);
            swprintf ( buffer, 100, L"_Period_spect_hsm%i.txt",hsmFr);
            wcsncat(processedFileName[HSM_SPE_FILENAME],buffer,100);
        }
        // определим названия файла для осциляций
        if(modFr != 0)
        {
            if(hsmFr == 0)
            {
                wcscpy(processedFileName[OSCIL_FILENAME],m_ConstPartOfDataFile);
                wchar_t buffer [100];
                swprintf(buffer, 100, L"_mod%i_pnt%i.tsp",modFr,carriagePos);
                wcsncat(processedFileName[OSCIL_FILENAME],buffer,100);
            }
            else
            {
                wcscpy(processedFileName[OSCIL_FILENAME],m_ConstPartOfDataFile);
                wchar_t buffer [100];
                swprintf(buffer, 100, L"_mod%i_pnt%i_hsm%i.tsp",modFr,carriagePos,hsmFr);
                wcsncat(processedFileName[OSCIL_FILENAME],buffer,100);
            }
            //файл для спектра частот
            wcscpy(processedFileName[CHOP_SPE_FILENAME],m_ConstPartOfDataFile);
            wchar_t buffer [100];
            swprintf ( buffer, 100, L"_Period_spect_mod%i.txt",modFr);
            wcsncat(processedFileName[CHOP_SPE_FILENAME],buffer,100);
        }
        return ;
    }

    bool CProcessRawEpigraphData::GetTamplateForProcessedFilePath(wchar_t constPartFilePath[],wchar_t filePath[])
    {
        wchar_t* fileName = NULL;
        fileName = wcsrchr(filePath,L'/');
        if(!fileName) return false;
        wcsncpy(constPartFilePath,
                filePath,
                wcslen(filePath)-wcslen(fileName));
        fileName = wcsrchr(constPartFilePath,L'/');
        if(!fileName) return false;
        wcsncat(constPartFilePath,fileName,wcslen(fileName));
        return true;
    }
    bool CProcessRawEpigraphData::GetDirectoryPath(wchar_t* dirPath,wchar_t* filePath)
    {
        wchar_t* fileName = NULL;
        fileName = wcsrchr(filePath,L'/');
        if(!fileName) return false;
        wcsncpy(dirPath,filePath,
                wcslen(filePath)-wcslen(fileName)+1);
        return true;
    }

    int CProcessRawEpigraphData::ProcessDirectory(wchar_t * dirPath)
    {
        char dirPathBuffer[PATH_MAX] = {NULL};
        wcstombs(dirPathBuffer,dirPath,sizeof(dirPathBuffer));
        WORD i(0);
        DIR *dir = opendir(dirPathBuffer);
        if(dir)
        {
           struct dirent *ent;
           int error(0);
           wchar_t filePath[PATH_MAX] = {NULL};
           wchar_t fileNameBuf[PATH_MAX] = {NULL};

           while((ent = readdir(dir)))
           {
              wcscpy(filePath,dirPath);
              if(!strstr(ent->d_name,".dat")) continue;
              mbstowcs(fileNameBuf,ent->d_name,sizeof(fileNameBuf));
              wcscat(filePath,fileNameBuf);
              if(!(error = ProcessFile(filePath)))
              {
                  i++;
                  if(m_Verbose == 1)wcout<<i<<"  "<<filePath<<" --> OK"<<endl;
              }
              else
              {
                  if(error == 46)
                  {
                      i++;
                      if(m_Verbose == 1) wcout<<i<<"  "<<filePath<<L" --> No pulse in DET channel"<<endl;
                  }
                  else
                  {
                      if(m_Verbose == 1) wcout<<i<<"  "<<filePath<<L" --> BAD error is "<<error<<endl;
                  }
              }
           }
        }
        else
            cout<<"Error opening directory\n";
        return i;
    }
    void CProcessRawEpigraphData::SetTamplateForProcessedFilePath(const wchar_t* tamplateForProcessFilePath)
    {
        wcscpy(m_ConstPartOfDataFile,tamplateForProcessFilePath);
    }
    void CProcessRawEpigraphData::SetDirectoryForProcessedFiles(wchar_t* dirPath)
    {
        //Очистим директорию от других файлов
        DeleteFilesInDirectory(dirPath);

        wchar_t someFiledInPrDir[PATH_MAX] = {NULL};
        wchar_t tamlPrFileBuf[PATH_MAX] = {NULL};
        if(*(dirPath+wcslen(dirPath)-1) == '/')
        {
            wcscpy(someFiledInPrDir,dirPath);
            wcscat(someFiledInPrDir,L"some_file");
            GetTamplateForProcessedFilePath(tamlPrFileBuf,someFiledInPrDir);
            SetTamplateForProcessedFilePath(tamlPrFileBuf);
        }
        else
        {
            wcscpy(someFiledInPrDir,dirPath);
            wcscat(someFiledInPrDir,L"/some_file");
            GetTamplateForProcessedFilePath(tamlPrFileBuf,someFiledInPrDir);
            SetTamplateForProcessedFilePath(tamlPrFileBuf);
        }
    }
    int CProcessRawEpigraphData::DeleteFilesInDirectory(wchar_t* dirPath)
    {
        char dirPathBuf[PATH_MAX] = {NULL};
        wcstombs(dirPathBuf,dirPath,sizeof(dirPathBuf));
        QDir dir(dirPathBuf);
        if(dir.exists())
        {
            QStringList strList = dir.entryList(QDir::Files);
            for(quint8 i(0);i<strList.length();i++)
            {
                dir.remove(strList.at(i));
                if(m_Verbose == 1)qDebug()<<strList.at(i)<<" --> Del";
            }
        }
        else
        {
            cout<<"Error opening directory\n";
        }

    }
