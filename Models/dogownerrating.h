#ifndef DOGOWNERRATING_H
#define DOGOWNERRATING_H

#include <QObject>

class DogOwnerRating : public QObject
{
    Q_OBJECT

public:
    int amountOfTimesOut() const { return m_amountOfTimesOut; }
    void setAmountOfTimesOut(int timesOut);
    int cumulativeTimeOut() const { return m_cumulativeTimeOut; }
    void setCumulativeTimeOut(int timesOut);
    int trainingSessionsAmount() const { return m_trainingSessionsAmount; }
    void setTrainingSessionsAmount(int sessionsAmount);

signals:
    void amountOfTimesOutChanged();
    void cumulativeTimeOutChanged();
    void trainingSessionsAmountChanged();

private:
    int m_amountOfTimesOut = 0;
    int m_cumulativeTimeOut = 0;
    int m_trainingSessionsAmount = 0;
};


#endif // DOGOWNERRATING_H
