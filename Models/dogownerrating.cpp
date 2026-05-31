#include "dogownerrating.h"

void DogOwnerRating::setAmount(int amount)
{
    if (m_amount != amount)
    {
        m_amount = amount;
        emit amountChanged();
    }
}