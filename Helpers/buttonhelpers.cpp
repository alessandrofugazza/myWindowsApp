#include "buttonhelpers.h"

#include <QLabel>
#include <QPushButton>
#include <QStringList>

QString formatButtonText(const QString &text, int maxLineLength)
{
    QStringList words = text.split(' ', Qt::SkipEmptyParts);
    QStringList lines;
    QString currentLine;

    for (const QString &word : words)
    {
        if (currentLine.isEmpty())
        {
            currentLine = word;
        }
        else if (currentLine.length() + 1 + word.length() <= maxLineLength)
        {
            currentLine += " " + word;
        }
        else
        {
            lines.append(currentLine);
            currentLine = word;
        }
    }

    if (!currentLine.isEmpty())
        lines.append(currentLine);

    return lines.join('\n');
}

QString formatSecondsAsHoursMinutes(qint64 seconds)
{
    if (seconds < 0)
        seconds = 0;

    qint64 totalMinutes = seconds / 60;
    qint64 hours = totalMinutes / 60;
    qint64 minutes = totalMinutes % 60;

    return QString("%1:%2")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'));
}

void updateButtonStatsLabels(QPushButton *btn)
{
    QLabel *clickCountLabel =
        btn->findChild<QLabel*>("clickCountLabel");

    QLabel *cumulativeTimeLabel =
        btn->findChild<QLabel*>("cumulativeTimeLabel");

    if (!clickCountLabel)
    {
        clickCountLabel = new QLabel(btn);
        clickCountLabel->setObjectName("clickCountLabel");
        clickCountLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        clickCountLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        clickCountLabel->setStyleSheet(
            "font-size: 8px;"
            "font-weight: 700;"
            "color: rgba(255,255,255,190);"
            "background: transparent;"
            );
    }

    if (!cumulativeTimeLabel)
    {
        cumulativeTimeLabel = new QLabel(btn);
        cumulativeTimeLabel->setObjectName("cumulativeTimeLabel");
        cumulativeTimeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        cumulativeTimeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cumulativeTimeLabel->setStyleSheet(
            "font-size: 8px;"
            "font-weight: 700;"
            "color: rgba(255,255,255,190);"
            "background: transparent;"
            );
    }

    int clickCount =
        btn->property("clickCount").toInt();

    qint64 cumulativeSeconds =
        btn->property("cumulativeSeconds").toLongLong();

    clickCountLabel->setText(QString::number(clickCount));
    cumulativeTimeLabel->setText(formatSecondsAsHoursMinutes(cumulativeSeconds));

    int labelHeight = 12;
    int labelY = btn->height() - labelHeight - 3;

    clickCountLabel->setGeometry(
        6,
        labelY,
        btn->width() / 2 - 8,
        labelHeight
        );

    cumulativeTimeLabel->setGeometry(
        btn->width() / 2,
        labelY,
        btn->width() / 2 - 6,
        labelHeight
        );

    clickCountLabel->raise();
    cumulativeTimeLabel->raise();

    clickCountLabel->show();
    cumulativeTimeLabel->show();
}