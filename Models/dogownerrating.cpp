#include "dogownerrating.h"

void DogOwnerRating::setAmountOfTimesOut(int amount)
{
    if (m_amountOfTimesOut != amount)
    {
        m_amountOfTimesOut = amount;
        emit amountOfTimesOutChanged();
    }
}

void DogOwnerRating::setCumulativeTimeOut(int cumulativeTime)
{
    if (m_cumulativeTimeOut != cumulativeTime)
    {
        m_cumulativeTimeOut = cumulativeTime;
        emit cumulativeTimeOutChanged();
    }
}

void DogOwnerRating::setTrainingSessionsAmount(int sessionsAmount)
{
    if (m_trainingSessionsAmount != sessionsAmount)
    {
        m_trainingSessionsAmount = sessionsAmount;
        emit trainingSessionsAmountChanged();
    }
}

