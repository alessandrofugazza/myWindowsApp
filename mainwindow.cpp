#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QWidget>
#include <QSet>
#include <algorithm>
#include <utility>
#include <QMessageBox>


#include <windows.h>

struct StudyButton
{
    QString name;
    int priority;
    QString appTitle;
};

QString formatButtonText(const QString &text, int maxLineLength = 20)
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , remainingTime(0)
{
    ui->setupUi(this);

    ui->viewsStack->setCurrentWidget(ui->productionView);

    // POLISH do this for everything?
    ui->currentChanceLbl->setText("settings not read yet");

    readSettings();

    connect(
        ui->btnColorTimeSpanSpinbox,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        [this](int)
        {
            QList<QPushButton*> buttons = findChildren<QPushButton*>();

            for (QPushButton *btn : std::as_const(buttons))
            {
                if (!btn->property("trackedColorButton").toBool())
                    continue;

                updateButtonColor(
                    btn,
                    btn->property("lastClicked").toDateTime()
                    );
            }

            writeSettings();
        }
        );

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qt-project.org/windows/cursors/images/openhandcursor_32.png"));
    trayIcon->setToolTip("My Windows App");
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::trayIconActivated);

    connect(ui->actionOtherView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentWidget(ui->otherView);
            });

    connect(ui->actionProductionView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentWidget(ui->productionView);
            });

    connect(ui->actionEdit_Options, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentWidget(ui->optionsView);
            });


    QList<StudyButton> buttons =
        {
            {"AHK", 5, "Chrome"},
            {"Anki", 6, "Chrome"},
            {"Automotive", 3, "Chrome"},
            {"C++", 4, "Chrome"},
            {"CB125R", 1, ""},
            {"Canophilia", 3, "Chrome"},
            {"Chrome Extensions Development", 3, "Chrome"},
            {"CompTIA", 1, "Chrome"},
            {"Data Science", 2, "Chrome"},
            {"Doblo", 1, ""},
            {"Excel", 7, "Chrome"},
            {"FTO", 1, "Chrome"},
            {"Finance", 1, "Chrome"},
            {"Flipper", 9, "Chrome"},
            {"GitHub Copilot", 2, "Chrome"},
            {"JavaScript", 5, "Chrome"},
            {"Law", 3, ""},
            {"Learn", 2, "Chrome"},
            {"Markdown", 2, "Chrome"},
            {"Obsidian", 2, "Chrome"},
            {"Power User", 7, "Chrome"},
            {"Project S", 3, "Chrome"},
            {"Project A", 4, "Chrome"},
            {"Project P", 5, "Chrome"},
            {"Prompt Engineering", 8, "Chrome"},
            {"Python", 4, "Chrome"},
            {"Swift", 4, "Chrome"},
            {"Web Development", 3, "Chrome"},
            {"Windows Development", 2, "Chrome"},
            {"iOS Development", 2, "Chrome"}
        };

    QFontMetrics metrics(ui->developWidget->font());

    int maxButtonWidth = 0;
    int maxButtonHeight = 0;

    for (const StudyButton &button : buttons)
    {
        QString wrappedText = formatButtonText(button.name);

        QSize textSize = metrics.size(
            Qt::TextShowMnemonic,
            wrappedText
            );

        maxButtonWidth = qMax(maxButtonWidth, textSize.width());
        maxButtonHeight = qMax(maxButtonHeight, textSize.height());
    }

    maxButtonWidth += 34;
    maxButtonHeight += 20;

    const int columnGap = 32;

    // IMPORTANT:
    // Do not put this layout directly on ui->developWidget.
    // developWidget already contains Designer-positioned widgets
    // such as taskIsDoneBtn and reopenLastTopicBtn.
    //
    // A layout installed directly on developWidget can overlap or
    // steal mouse events from those buttons, making only part of
    // the visible button clickable.
    QWidget *studyButtonsContainer = new QWidget(ui->developWidget);
    studyButtonsContainer->setObjectName("studyButtonsContainer");
    studyButtonsContainer->setGeometry(20, 20, 1720, 580);

    QHBoxLayout *mainLayout = new QHBoxLayout(studyButtonsContainer);

    mainLayout->setSpacing(columnGap);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QSet<int> priorities;

    for (const StudyButton &button : buttons)
    {
        priorities.insert(button.priority);
    }

    QList<int> sortedPriorities = priorities.values();

    std::sort(sortedPriorities.begin(), sortedPriorities.end());

    for (int priority : sortedPriorities)
    {
        QWidget *columnWidget = new QWidget(studyButtonsContainer);

        columnWidget->setMinimumWidth(maxButtonWidth);
        columnWidget->setMaximumWidth(maxButtonWidth);

        QVBoxLayout *columnLayout = new QVBoxLayout(columnWidget);

        columnLayout->setSpacing(10);
        columnLayout->setContentsMargins(0, 0, 0, 0);
        columnLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        QLabel *title = new QLabel(
            QString("Priority %1").arg(priority),
            columnWidget
            );

        title->setMinimumWidth(maxButtonWidth);
        title->setMaximumWidth(maxButtonWidth);
        title->setAlignment(Qt::AlignCenter);

        columnLayout->addWidget(title);

        priorityLayouts.insert(priority, columnLayout);

        mainLayout->addWidget(columnWidget);
    }

    int studyButtonIndex = 0;

    for (const StudyButton &button : buttons)
    {
        QPushButton *btn = new QPushButton(
            formatButtonText(button.name),
            studyButtonsContainer
            );

        QFont font = btn->font();

        font.setWeight(QFont::DemiBold);

        btn->setFont(font);

        btn->setObjectName(button.name);

        btn->setMinimumSize(maxButtonWidth, maxButtonHeight);
        btn->setMaximumSize(maxButtonWidth, maxButtonHeight);

        btn->setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed
            );

        btn->setProperty("trackedColorButton", true);
        btn->setProperty("priority", button.priority);
        btn->setProperty("appTitle", button.appTitle);
        btn->setProperty("originalIndex", studyButtonIndex);
        ++studyButtonIndex;

        priorityLayouts[button.priority]->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, btn]()
                {

                    checkTaskWithChance();

                    QDateTime now = QDateTime::currentDateTime();

                    btn->setProperty("lastClicked", now);

                    updateButtonColor(btn, now);

                    writeSettings();

                    int priority = btn->property("priority").toInt();

                    QVBoxLayout *columnLayout =
                        priorityLayouts.value(priority, nullptr);

                    if (columnLayout != nullptr)
                    {
                        columnLayout->removeWidget(btn);

                        int insertIndex = columnLayout->count() - 1;

                        if (insertIndex < 0)
                            insertIndex = 0;

                        columnLayout->insertWidget(insertIndex, btn);
                    }

                    QString appTitle =
                        btn->property("appTitle").toString();

                    QString instanceName =
                        btn->objectName();

                    if (!appTitle.isEmpty())
                    {
                        if (appTitle == "Chrome")
                        {
                            activateWindowByTitle(instanceName);
                        }
                    }
                });
    }

    for (QVBoxLayout *columnLayout : std::as_const(priorityLayouts))
    {
        columnLayout->addStretch();
    }

    {
        QSettings settings;

        QList<QPushButton*> createdButtons = findChildren<QPushButton*>();

        for (QPushButton *btn : std::as_const(createdButtons))
        {
            if (!btn->property("trackedColorButton").toBool())
                continue;

            QDateTime lastClicked = settings.value(
                                                QString("studyButtons/%1/lastClicked")
                                                    .arg(btn->objectName())
                                                ).toDateTime();

            if (!lastClicked.isValid())
                continue;

            btn->setProperty("lastClicked", lastClicked);

            updateButtonColor(btn, lastClicked);
        }

        for (int priority : sortedPriorities)
        {
            QVBoxLayout *columnLayout =
                priorityLayouts.value(priority, nullptr);

            if (!columnLayout)
                continue;

            QList<QPushButton*> priorityButtons;

            for (int i = 0; i < columnLayout->count(); ++i)
            {
                QWidget *widget =
                    columnLayout->itemAt(i)->widget();

                QPushButton *btn =
                    qobject_cast<QPushButton*>(widget);

                if (!btn)
                    continue;

                if (!btn->property("trackedColorButton").toBool())
                    continue;

                priorityButtons.append(btn);
            }

            std::sort(
                priorityButtons.begin(),
                priorityButtons.end(),
                [](QPushButton *a, QPushButton *b)
                {
                    QDateTime aClicked =
                        a->property("lastClicked").toDateTime();

                    QDateTime bClicked =
                        b->property("lastClicked").toDateTime();

                    if (aClicked.isValid() != bClicked.isValid())
                        return !aClicked.isValid();

                    if (aClicked.isValid() && bClicked.isValid() && aClicked != bClicked)
                        return aClicked < bClicked;

                    return
                        a->property("originalIndex").toInt()
                        <
                        b->property("originalIndex").toInt();
                }
                );

            for (QPushButton *btn : priorityButtons)
            {
                columnLayout->removeWidget(btn);

                int insertIndex = columnLayout->count() - 1;

                if (insertIndex < 0)
                    insertIndex = 0;

                columnLayout->insertWidget(insertIndex, btn);
            }
        }
    }

    connect(&timer, &QTimer::timeout,
            this, &MainWindow::updateCountdown);

    QList<QPushButton*> allButtons = findChildren<QPushButton*>();

    for (QPushButton *btn : allButtons)
    {
        btn->setCursor(Qt::PointingHandCursor);
    }

    // Keep Designer buttons above any runtime-created widgets.
    ui->taskIsDoneBtn->raise();
    ui->reopenLastTopicBtn->raise();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::checkTaskWithChance()
{
    double chance = currentChance();

    // DEBUG

    QDateTime now = QDateTime::currentDateTime();

    qint64 elapsedSeconds =
        chanceStartTime.secsTo(now);

    qint64 minutes = elapsedSeconds / 60;
    qint64 seconds = elapsedSeconds % 60;

    qDebug() << "========================";
    qDebug() << "Current time:"
             << now.toString("yyyy-MM-dd hh:mm:ss");

    qDebug() << "Timer started at:"
             << chanceStartTime.toString("yyyy-MM-dd hh:mm:ss");

    qDebug() << "Elapsed:"
             << QString("%1m %2s")
                    .arg(minutes)
                    .arg(seconds);

    qDebug() << "Current chance:"
             << QString::number(chance * 100.0, 'f', 1) + "%";


    //

    double roll = QRandomGenerator::global()->generateDouble();

    if (roll < chance)
    {
        doTaskTriggeredStuff();

    } else {
        updateCurrentChanceLabel();
    }
}

void MainWindow::doTaskTriggeredStuff()
{
    ui->currentChanceLbl->setText(
        QString("Task triggered!")
        );
    taskIsTriggered = true;
    ui->taskIsDoneBtn->setEnabled(true);
    ui->taskIsDoneBtn->setText("TASK COMPLETED");
}

void MainWindow::updateCurrentChanceLabel()
{
    if (taskIsTriggered)
    {
        return;
    }
    double chance = currentChance();

    ui->currentChanceLbl->setText(
        QString("%1%").arg(chance * 100, 0, 'f', 1)
        );

    ui->chanceProgressBar->setValue(static_cast<int>(chance * 100));

    if (chance == 1.0)
    {
        doTaskTriggeredStuff();
    }
}

void MainWindow::updateButtonColor(QPushButton *btn, QDateTime clickedTime)
{
    if (!clickedTime.isValid())
    {
        btn->setStyleSheet("");
        return;
    }

    qint64 secondsElapsed =
        clickedTime.secsTo(QDateTime::currentDateTime());

    int timeSpanMinutes =
        ui->btnColorTimeSpanSpinbox->value();

    if (timeSpanMinutes <= 0)
        timeSpanMinutes = 1;

    double maxSeconds =
        static_cast<double>(timeSpanMinutes) * 60.0;

    double t = qMin(secondsElapsed / maxSeconds, 1.0);

    int startR = 76;
    int startG = 175;
    int startB = 80;

    int endR = 183;
    int endG = 28;
    int endB = 28;

    int r = static_cast<int>(startR + (endR - startR) * t);
    int g = static_cast<int>(startG + (endG - startG) * t);
    int b = static_cast<int>(startB + (endB - startB) * t);

    QString textColor =
        t < 0.45
            ? "rgb(15,15,15)"
            : "rgb(245,245,245)";

    btn->setStyleSheet(
        QString(
            "QPushButton {"
            "background-color: rgb(%1,%2,%3);"
            "color: %4;"
            "border: 1px solid rgba(255,255,255,45);"
            "border-radius: 4px;"
            "}"
            )
            .arg(r)
            .arg(g)
            .arg(b)
            .arg(textColor)
        );
}

// ON FOCUS

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        QList<QPushButton*> buttons = findChildren<QPushButton*>();

        for (QPushButton *btn : std::as_const(buttons))
        {
            if (!btn->property("trackedColorButton").toBool())
                continue;

            updateButtonColor(
                btn,
                btn->property("lastClicked").toDateTime()
                );
        }

        updateCurrentChanceLabel();
        updateMoveTopicChanceLabel();
    }

    return QMainWindow::event(event);
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        if (isMinimized())
            showNormal();

        show();
        raise();
        activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    QWidget::closeEvent(event);
}

void MainWindow::readSettings()
{
    QSettings settings;

    ui->notConcluded->setPlainText(
        settings.value("notConcluded", "").toString()
        );

    ui->studyNotes->setPlainText(
        settings.value("studyNotes", "").toString()
        );

    lastOpenedTopic =
        settings.value("lastOpenedTopic", "").toString();

    chanceStartTime = settings.value(
                                  "chance/startTime",
                                  QDateTime::currentDateTime()
                                  ).toDateTime();

    ui->btnColorTimeSpanSpinbox->setValue(
        settings.value("buttonColor/timeSpanMinutes", 120).toInt()
        );

    if (!chanceStartTime.isValid())
        chanceStartTime = QDateTime::currentDateTime();

    updateCurrentChanceLabel();

    moveTopicChanceStartTime = settings.value(
                                           "moveTopicChance/startTime",
                                           QDateTime::currentDateTime()
                                           ).toDateTime();

    if (!moveTopicChanceStartTime.isValid())
        moveTopicChanceStartTime = QDateTime::currentDateTime();

    updateMoveTopicChanceLabel();
    taskIsTriggered = settings.value(
                                  "taskIsTriggered", false).toBool();
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.setValue(
        "notConcluded",
        ui->notConcluded->toPlainText()
        );

    settings.setValue(
        "studyNotes",
        ui->studyNotes->toPlainText()
        );

    settings.setValue(
        "lastOpenedTopic",
        lastOpenedTopic
        );

    settings.setValue("chance/startTime", chanceStartTime);

    settings.setValue(
        "buttonColor/timeSpanMinutes",
        ui->btnColorTimeSpanSpinbox->value()
        );

    settings.setValue(
        "moveTopicChance/startTime",
        moveTopicChanceStartTime
        );

    settings.setValue(
        "taskIsTriggered",
        taskIsTriggered
        );

    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    for (QPushButton *btn : std::as_const(buttons))
    {
        if (!btn->property("trackedColorButton").toBool())
            continue;

        QDateTime lastClicked =
            btn->property("lastClicked").toDateTime();

        QString key = QString("studyButtons/%1/lastClicked")
                          .arg(btn->objectName());

        if (lastClicked.isValid())
        {
            settings.setValue(key, lastClicked);
        }
        else
        {
            settings.remove(key);
        }
    }
}

bool MainWindow::activateWindowByTitle(const QString &target)
{
    HWND hwnd = FindWindowA(nullptr, nullptr);

    while (hwnd != nullptr)
    {
        char title[512];

        GetWindowTextA(hwnd, title, sizeof(title));

        if (title[0] != '\0')
        {
            QString t = QString::fromLatin1(title);

            if (t == target)
            {
                ShowWindow(hwnd, SW_MAXIMIZE);
                SetForegroundWindow(hwnd);
                lastOpenedTopic = target;

                return true;
            }
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    qDebug()
        << "Window with title containing"
        << target
        << "not found.";

    return false;
}

void MainWindow::handleWindowButton(
    QPushButton *btn,
    const QString &target
    )
{
    Q_UNUSED(btn);

    activateWindowByTitle(target);
}

void MainWindow::on_activateMainBtn_clicked()
{
    handleWindowButton(ui->activateMainBtn, "main");
}

void MainWindow::on_activateLeftBtn_clicked()
{
    handleWindowButton(ui->activateLeftBtn, "left");
}

void MainWindow::on_activateRightBtn_clicked()
{
    handleWindowButton(ui->activateRightBtn, "right");
}

void MainWindow::on_activateWebtestRightBtn_clicked()
{
    handleWindowButton(ui->activateWebtestRightBtn, "webtest");
}

void MainWindow::on_activateWebtestLeftBtn_clicked()
{
    handleWindowButton(ui->activateWebtestLeftBtn, "webtest");
}

void MainWindow::on_activateRfcBtn_clicked()
{
    handleWindowButton(ui->activateRfcBtn, "reference");
}

void MainWindow::updateCountdown()
{
    remainingTime--;

    if (remainingTime >= 0)
    {
        ui->timeLeftLabel->setText(
            QString::number(remainingTime)
            );
    }

    if (remainingTime <= 0)
    {
        timer.stop();

        trayIcon->showMessage(
            "Timer finished",
            "Your countdown ended.",
            QSystemTrayIcon::Information,
            5000
            );

        QMessageBox::information(
            this,
            "Timer finished",
            "Your countdown ended."
            );
    }
}

void MainWindow::on_startTimerBtn_clicked()
{
    remainingTime = 5;

    ui->timeLeftLabel->setText(
        QString::number(remainingTime)
        );

    timer.start(1000);
}

void MainWindow::resetChanceTimer()
{
    // chanceTimer.restart();
    chanceStartTime = QDateTime::currentDateTime();
    writeSettings();

}

double MainWindow::currentChance() const
{
    if (!chanceStartTime.isValid())
        return 0.0;

    constexpr double maxSeconds = 25.0 * 60.0;

    double elapsedSeconds =
        chanceStartTime.secsTo(QDateTime::currentDateTime());

    double progress = elapsedSeconds / maxSeconds;

    progress = qBound(0.0, progress, 1.0);

    return progress;
}

void MainWindow::on_taskIsDoneBtn_clicked()
{
    resetChanceTimer();

    ui->taskIsDoneBtn->setEnabled(false);
    ui->taskIsDoneBtn->setText("TASK IS DONE");

    updateCurrentChanceLabel();
    taskIsTriggered = false;
    writeSettings();

}

void MainWindow::on_reopenLastTopicBtn_clicked()
{
    if (lastOpenedTopic.isEmpty())
    {
        QMessageBox::information(
            this,
            "No topic",
            "No topic was opened yet."
            );
        return;
    }

    if (!activateWindowByTitle(lastOpenedTopic))
    {
        QMessageBox::warning(
            this,
            "Topic not found",
            QString("Could not find a window with title '%1'.").arg(lastOpenedTopic)
            );
    }
}

void MainWindow::resetMoveTopicChanceTimer()
{
    moveTopicChanceStartTime = QDateTime::currentDateTime();
    writeSettings();
}

double MainWindow::currentMoveTopicChance() const
{
    if (!moveTopicChanceStartTime.isValid())
        return 0.0;

    constexpr double maxSeconds = 30.0 * 60.0;

    double elapsedSeconds =
        moveTopicChanceStartTime.secsTo(QDateTime::currentDateTime());

    double progress = elapsedSeconds / maxSeconds;

    return qBound(0.0, progress, 1.0);
}

void MainWindow::updateMoveTopicChanceLabel()
{
    double chance = currentMoveTopicChance();

    ui->moveTopicChanceLbl->setText(
        QString("%1%").arg(chance * 100, 0, 'f', 1)
        );

    ui->moveTopicProgressBar->setValue(
        static_cast<int>(chance * 100)
        );

    if (chance == 1.0)
    {
        ui->moveTopicChanceLbl->setText("Move topic triggered!");
        ui->moveTopicBtn->setEnabled(true);
        ui->moveTopicBtn->setText("MOVE TOPIC");
    }
}

void MainWindow::on_moveTopicBtn_clicked()
{
    resetMoveTopicChanceTimer();

    ui->moveTopicBtn->setEnabled(false);
    ui->moveTopicBtn->setText("MOVE TOPIC");

    updateMoveTopicChanceLabel();

    writeSettings();

}

