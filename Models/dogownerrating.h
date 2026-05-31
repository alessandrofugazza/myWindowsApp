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

signals:
    void amountOfTimesOutChanged();
    void cumulativeTimeOutChanged();

private:
    int m_amountOfTimesOut = 0;
    int m_cumulativeTimeOut = 0;
};


#endif // DOGOWNERRATING_H
