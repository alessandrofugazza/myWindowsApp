#ifndef DOGOWNERRATING_H
#define DOGOWNERRATING_H

#include <QObject>

class DogOwnerRating : public QObject
{
    Q_OBJECT

public:
    int amount() const { return m_amount; }
    void setAmount(int amount);

signals:
    void amountChanged();

private:
    int m_amount = 0;
};


#endif // DOGOWNERRATING_H
