#ifndef BUTTONHELPERS_H
#define BUTTONHELPERS_H

include <QString>

class QPushButton;

QString formatButtonText(const QString &text, int maxLineLength = 20);

QString formatSecondsAsHoursMinutes(qint64 seconds);

void updateButtonStatsLabels(QPushButton *btn);


#endif // BUTTONHELPERS_H
