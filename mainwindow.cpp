#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Helpers/buttonhelpers.h"
#include "Models/studybutton.h"
#include "Data/studybuttonrepository.h"

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
#include "Models/dogownerrating.h"

// constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // develop

    m_amountOfTimesOut = ui->amountOfTimesOutSb->value();
    m_cumulativeTimeOut = ui->cumulativeTimeOutSb->value();

    connect(
        ui->amountOfTimesOutSb,
        &QSpinBox::valueChanged,
        this,
        &MainWindow::amountOfTimesOutChanged
        );

    connect(
        ui->cumulativeTimeOutSb,
        &QSpinBox::valueChanged,
        this,
        &MainWindow::cumulativeTimeOutChanged
        );

    connect(this, &MainWindow::dogOwnerRatingChanged, this, [this]()
            {
                qDebug() << "RECEIVING Dog owner rating changed. New score:" << m_dogOwnerRatingScore;
                ui->dogOwnerRatingScoreLbl->setText(
                    QString::number(m_dogOwnerRatingScore)
                    );
            });

    calculateDogOwnerRating();

    // signals and slots logic

    connect(
        ui->reopenLastTopicBtn,
        &QPushButton::clicked,
        this,
        &MainWindow::onReopenLastTopicBtnClicked
        );

    connect(
        ui->resetTopicsBtn,
        &QPushButton::clicked,
        this,
        &MainWindow::onResetTopicsBtnClicked
        );

    connect(ui->debugBtn,
            &QPushButton::clicked,
            this,
            [this]()
            {
                qDebug() << "progressIsBeingTracked:" << progressIsBeingTracked;


                QList<QPushButton*> buttons = findChildren<QPushButton*>();

                for (QPushButton *btn : std::as_const(buttons))
                {
                    if (!btn->property("trackedColorButton").toBool())
                        continue;

                    if (!btn->property("selected").toBool())
                        continue;

                    QDateTime lastClicked =
                        btn->property("lastClicked").toDateTime();

                    QDateTime now =
                        QDateTime::currentDateTime();

                    qint64 pausedSeconds =
                        btn->property("pausedSeconds").toLongLong();

                    bool isPaused =
                        btn->property("isPaused").toBool();

                    QDateTime pauseStartedAt =
                        btn->property("pauseStartedAt").toDateTime();

                    qint64 pausedSecondsIncludingCurrentPause =
                        pausedSeconds;

                    if (isPaused && pauseStartedAt.isValid())
                    {
                        qint64 currentPauseSeconds =
                            pauseStartedAt.secsTo(now);

                        if (currentPauseSeconds < 0)
                            currentPauseSeconds = 0;

                        pausedSecondsIncludingCurrentPause += currentPauseSeconds;
                    }

                    qint64 rawSeconds =
                        lastClicked.isValid()
                            ? lastClicked.secsTo(now)
                            : 0;

                    qint64 activeSeconds =
                        rawSeconds - pausedSecondsIncludingCurrentPause;

                    if (activeSeconds < 0)
                        activeSeconds = 0;

                    qDebug() << "Selected topic debug:"
                             << btn->objectName();

                    qDebug() << "isPaused:"
                             << isPaused;

                    qDebug() << "lastClicked:"
                             << lastClicked.toString("yyyy-MM-dd HH:mm:ss");

                    qDebug() << "pauseStartedAt:"
                             << pauseStartedAt.toString("yyyy-MM-dd HH:mm:ss");

                    qDebug() << "stored pausedSeconds:"
                             << pausedSeconds;

                    qDebug() << "pausedSeconds including current pause:"
                             << pausedSecondsIncludingCurrentPause;

                    qDebug() << "rawSeconds since lastClicked:"
                             << rawSeconds;

                    qDebug() << "activeSeconds excluding pauses:"
                             << activeSeconds;

                    qDebug() << "cumulativeSeconds:"
                             << btn->property("cumulativeSeconds").toLongLong();

                    return;
                }

                qDebug() << "No selected tracked topic found";
            }
            );

    HWND hwnd = reinterpret_cast<HWND>(winId());

    if (!RegisterHotKey(hwnd, FINISH_HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'D')
)
    {
        qDebug() << "RegisterHotKey for Shift + Alt + D failed. Error:" << GetLastError();
    }
    else
    {
        qDebug() << "RegisterHotKey for Shift + Alt + D succeeded";
    }

    if (!RegisterHotKey(hwnd, PAUSE_HOTKEY_ID, MOD_CONTROL | MOD_ALT | MOD_SHIFT, 'F'))
    {
        qDebug() << "RegisterHotKey for Shift + Alt + F failed. Error:" << GetLastError();
    }
    else
    {
        qDebug() << "RegisterHotKey for Shift + Alt + F succeeded";
    }

    ui->viewsStack->setCurrentWidget(ui->productionView);

    ui->currentChanceLbl->setText("settings not read yet");
    ui->trackingStartedAtLbl->setText("Not started");

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
                    buttonColorReferenceTime(btn)
                    );

                updateButtonStatsLabels(btn);
            }

            writeSettings();
        }
        );

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qt-project.org/windows/cursors/images/openhandcursor_32.png"));
    trayIcon->setToolTip("My Windows App");
    trayIcon->show();

    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &MainWindow::trayIconActivated
        );

    connect(
        ui->actionOtherView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->otherView);
        }
        );

    connect(
        ui->actionProductionView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->productionView);
        }
        );

    connect(
        ui->actionDevelopView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->dogOwnerRatingView);
        }
        );

    connect(
        ui->actionEdit_Options,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->optionsView);
        }
        );

    connect(
        ui->actionCoursePracticeView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->coursePracticeView);
        }
        );


    setupStudyButtons();

    restoreStudyButtonSettings();

    QList<QPushButton*> allButtons = findChildren<QPushButton*>();

    for (QPushButton *btn : allButtons)
    {
        btn->setCursor(Qt::PointingHandCursor);
    }
}

// DESTRUCTOR
MainWindow::~MainWindow()
{
    HWND hwnd = reinterpret_cast<HWND>(winId());

    UnregisterHotKey(
        hwnd,
        FINISH_HOTKEY_ID
        );

    UnregisterHotKey(
        hwnd,
        PAUSE_HOTKEY_ID
        );

    delete ui;
}

QDateTime MainWindow::buttonColorReferenceTime(QPushButton *btn) const
{
    if (btn == nullptr)
        return QDateTime();

    bool selected =
        btn->property("selected").toBool();

    if (progressIsBeingTracked && selected)
    {
        QDateTime lastClicked =
            btn->property("lastClicked").toDateTime();

        if (!lastClicked.isValid())
            return QDateTime();

        qint64 pausedSeconds =
            btn->property("pausedSeconds").toLongLong();

        bool isPaused =
            btn->property("isPaused").toBool();

        QDateTime pauseStartedAt =
            btn->property("pauseStartedAt").toDateTime();

        QDateTime effectiveEnd =
            QDateTime::currentDateTime();

        if (isPaused && pauseStartedAt.isValid())
        {
            effectiveEnd = pauseStartedAt;
        }

        qint64 activeElapsedSeconds =
            lastClicked.secsTo(effectiveEnd) - pausedSeconds;

        if (activeElapsedSeconds < 0)
            activeElapsedSeconds = 0;

        return QDateTime::currentDateTime().addSecs(-activeElapsedSeconds);
    }

    return btn->property("lastDone").toDateTime();
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

bool MainWindow::nativeEvent(
    const QByteArray &eventType,
    void *message,
    qintptr *result
    )
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *msg = static_cast<MSG *>(message);

    if (msg->message != WM_HOTKEY)
    {
        return QMainWindow::nativeEvent(
            eventType,
            message,
            result
            );
    }


    qDebug() << "WM_HOTKEY received";

    switch (msg->wParam)
    {
    case FINISH_HOTKEY_ID:
    {
        qDebug() << "Shift + Alt + D pressed globally";

        QPushButton *selectedButton = nullptr;

        QList<QPushButton*> buttons = findChildren<QPushButton*>();

        for (QPushButton *btn : std::as_const(buttons))
        {
            if (!btn->property("trackedColorButton").toBool())
                continue;

            if (!btn->property("selected").toBool())
                continue;

            selectedButton = btn;
            break;
        }

        if (selectedButton == nullptr)
        {
            QMessageBox::information(
                this,
                "No selected topic",
                "No topic button was clicked yet."
                );

            qDebug() << "Global hotkey pressed, but no selected topic exists";

            return true;
        }

        QDateTime lastClicked =
            selectedButton->property("lastClicked").toDateTime();

        if (!lastClicked.isValid())
        {
            QMessageBox::warning(
                this,
                "No last clicked time",
                QString("Selected topic '%1' has no valid lastClicked time.")
                    .arg(selectedButton->objectName())
                );

            qDebug() << "Global hotkey pressed, but selected topic has no valid lastClicked";

            return true;
        }

        QDateTime lastDone =
            QDateTime::currentDateTime();

        selectedButton->setProperty("lastDone", lastDone);

        qint64 pausedSeconds =
            selectedButton->property("pausedSeconds").toLongLong();

        bool isPaused =
            selectedButton->property("isPaused").toBool();

        QDateTime pauseStartedAt =
            selectedButton->property("pauseStartedAt").toDateTime();

        if (isPaused && pauseStartedAt.isValid())
        {
            qint64 finalPausedSeconds =
                pauseStartedAt.secsTo(lastDone);

            if (finalPausedSeconds < 0)
                finalPausedSeconds = 0;

            pausedSeconds += finalPausedSeconds;

            qDebug() << "Finish pressed while topic was paused";
            qDebug() << "Final paused seconds added:" << finalPausedSeconds;
        }

        qint64 elapsedSeconds =
            lastClicked.secsTo(lastDone) - pausedSeconds;

        if (elapsedSeconds < 0)
            elapsedSeconds = 0;

        qDebug() << "Finish elapsed raw seconds:"
                 << lastClicked.secsTo(lastDone);

        qDebug() << "Finish elapsed paused seconds excluded:"
                 << pausedSeconds;

        qDebug() << "Finish elapsed active seconds:"
                 << elapsedSeconds;

        qint64 cumulativeSeconds =
            selectedButton->property("cumulativeSeconds").toLongLong();

        cumulativeSeconds += elapsedSeconds;

        selectedButton->setProperty("cumulativeSeconds", cumulativeSeconds);
        selectedButton->setProperty("isPaused", false);
        selectedButton->setProperty("pauseStartedAt", QDateTime());
        selectedButton->setProperty("pausedSeconds", 0);

        qint64 elapsedHours =
            elapsedSeconds / 3600;

        qint64 elapsedMinutes =
            (elapsedSeconds % 3600) / 60;

        qint64 remainingSeconds =
            elapsedSeconds % 60;

        QString elapsedText =
            QString("%1h %2m %3s")
                .arg(elapsedHours)
                .arg(elapsedMinutes)
                .arg(remainingSeconds);

        selectedButton->setProperty("selected", false);

        progressIsBeingTracked = false;

        updateButtonColor(
            selectedButton,
            buttonColorReferenceTime(selectedButton)
            );

        updateButtonStatsLabels(selectedButton);

        writeSettings();

        qDebug() << "Global hotkey pressed";
        qDebug() << "Last done saved for" << selectedButton->objectName();
        qDebug() << "Elapsed between lastClicked and lastDone:" << elapsedText;
        QMessageBox::information(
            this,
            "Topic finished",
            QString("You spent %1 on '%2'.")
                .arg(elapsedText)
                .arg(selectedButton->objectName())
            );

        return true;
    }
    case PAUSE_HOTKEY_ID:
    {
        qDebug() << "Shift + Alt + F pressed globally";

        QPushButton *selectedButton = nullptr;

        QList<QPushButton*> buttons = findChildren<QPushButton*>();

        for (QPushButton *btn : std::as_const(buttons))
        {
            if (!btn->property("trackedColorButton").toBool())
                continue;

            if (!btn->property("selected").toBool())
                continue;

            selectedButton = btn;
            break;
        }

        if (selectedButton == nullptr)
        {
            qDebug() << "Pause hotkey ignored: no selected topic exists";
            return true;
        }

        if (!progressIsBeingTracked)
        {
            qDebug() << "Pause hotkey ignored: progressIsBeingTracked is false";
            return true;
        }

        QDateTime lastClicked =
            selectedButton->property("lastClicked").toDateTime();

        if (!lastClicked.isValid())
        {
            qDebug() << "Pause hotkey ignored: selected topic has no valid lastClicked"
                     << selectedButton->objectName();

            return true;
        }

        QDateTime now =
            QDateTime::currentDateTime();

        bool isPaused =
            selectedButton->property("isPaused").toBool();

        if (!isPaused)
        {
            selectedButton->setProperty("isPaused", true);
            selectedButton->setProperty("pauseStartedAt", now);

            qint64 pausedSeconds =
                selectedButton->property("pausedSeconds").toLongLong();

            qint64 activeSecondsSoFar =
                lastClicked.secsTo(now) - pausedSeconds;

            if (activeSecondsSoFar < 0)
                activeSecondsSoFar = 0;

            qDebug() << "Topic paused:"
                     << selectedButton->objectName();

            qDebug() << "Pause started at:"
                     << now.toString("yyyy-MM-dd HH:mm:ss");

            qDebug() << "Active seconds before pause:"
                     << activeSecondsSoFar;

            qDebug() << "Paused seconds already stored:"
                     << pausedSeconds;
        }
        else
        {
            QDateTime pauseStartedAt =
                selectedButton->property("pauseStartedAt").toDateTime();

            qint64 pausedSecondsToAdd = 0;

            if (pauseStartedAt.isValid())
            {
                pausedSecondsToAdd =
                    pauseStartedAt.secsTo(now);

                if (pausedSecondsToAdd < 0)
                    pausedSecondsToAdd = 0;
            }

            qint64 pausedSeconds =
                selectedButton->property("pausedSeconds").toLongLong();

            pausedSeconds += pausedSecondsToAdd;

            selectedButton->setProperty("isPaused", false);
            selectedButton->setProperty("pauseStartedAt", QDateTime());
            selectedButton->setProperty("pausedSeconds", pausedSeconds);

            qint64 activeSecondsSoFar =
                lastClicked.secsTo(now) - pausedSeconds;

            if (activeSecondsSoFar < 0)
                activeSecondsSoFar = 0;

            qDebug() << "Topic resumed:"
                     << selectedButton->objectName();

            qDebug() << "Pause ended at:"
                     << now.toString("yyyy-MM-dd HH:mm:ss");

            qDebug() << "Paused seconds added:"
                     << pausedSecondsToAdd;

            qDebug() << "Total paused seconds for this run:"
                     << pausedSeconds;

            qDebug() << "Active seconds so far, excluding pauses:"
                     << activeSecondsSoFar;
        }

        updateButtonColor(
            selectedButton,
            buttonColorReferenceTime(selectedButton)
            );

        updateButtonStatsLabels(selectedButton);

        writeSettings();

        QMessageBox::information(
            this,
            isPaused ? "Topic paused" : "Topic resumed",
            QString("You have %1 '%2'.")
                .arg(isPaused ? "paused" : "resumed")
                .arg(selectedButton->objectName())
            );

        return true;
    }

    default:
    {
        qDebug() << "Unknown hotkey ID:" << msg->wParam;
        break;
    }


    }




    return QMainWindow::nativeEvent(
        eventType,
        message,
        result
        );
}

double MainWindow::calculateCurrentTaskChance() const
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

void MainWindow::checkTaskWithChance()
{
    double chance = calculateCurrentTaskChance();

    QDateTime now = QDateTime::currentDateTime();

    qint64 elapsedSeconds =
        chanceStartTime.secsTo(now);

    double roll = QRandomGenerator::global()->generateDouble();

    if (roll < chance)
    {
        doTaskTriggeredStuff();
    }
    else
    {
        updateCurrentChanceLabel();
    }
}

void MainWindow::doTaskTriggeredStuff()
{
    bool isTask =
        QRandomGenerator::global()->bounded(2) == 0;

    if (isTask)
    {
        ui->currentChanceLbl->setText("Task triggered!");
        ui->taskIsDoneBtn->setText("TASK COMPLETED");
    }
    else
    {
        ui->currentChanceLbl->setText("Move topic triggered!");
        ui->taskIsDoneBtn->setText("MOVE TOPIC");
    }

    taskIsTriggered = true;
    ui->taskIsDoneBtn->setEnabled(true);
}

void MainWindow::updateCurrentChanceLabel()
{
    if (taskIsTriggered)
    {
        return;
    }

    double chance = calculateCurrentTaskChance();

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
    bool selected =
        btn->property("selected").toBool();

    int borderWidth =
        selected ? 3 : 1;

    int borderAlpha =
        selected ? 230 : 80;

    if (!clickedTime.isValid())
    {
        btn->setStyleSheet(
            QString(
                "QPushButton {"
                "font-weight: 600;"
                "border: %1px solid rgba(255,255,255,%2);"
                "border-radius: 4px;"
                "padding: 4px 8px;"
                "}"
                )
                .arg(borderWidth)
                .arg(borderAlpha)
            );

        updateButtonStatsLabels(btn);

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

    int r, g, b;

    if (t < 0.5)
    {
        double localT = t / 0.5;

        r = static_cast<int>(46  + (202 - 46)  * localT);
        g = static_cast<int>(125 + (138 - 125) * localT);
        b = static_cast<int>(50  + (4   - 50)  * localT);
    }
    else
    {
        double localT = (t - 0.5) / 0.5;

        r = static_cast<int>(202 + (183 - 202) * localT);
        g = static_cast<int>(138 + (28  - 138) * localT);
        b = static_cast<int>(4   + (28  - 4)   * localT);
    }

    btn->setStyleSheet(
        QString(
            "QPushButton {"
            "background-color: rgb(%1,%2,%3);"
            "color: rgb(255,255,255);"
            "font-weight: 600;"
            "border: %4px solid rgba(255,255,255,%5);"
            "border-radius: 4px;"
            "padding: 4px 8px;"
            "}"
            )
            .arg(r)
            .arg(g)
            .arg(b)
            .arg(borderWidth)
            .arg(borderAlpha)
        );

    updateButtonStatsLabels(btn);
}

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
                buttonColorReferenceTime(btn)
                );

            updateButtonStatsLabels(btn);
        }

        updateCurrentChanceLabel();
    }

    return QMainWindow::event(event);
}

void MainWindow::readSettings()
{
    QSettings settings;

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

    taskIsTriggered = settings.value(
                                  "taskIsTriggered",
                                  false
                                  ).toBool();

    progressIsBeingTracked = settings.value(
                                         "studyButtons/progressIsBeingTracked",
                                         false
                                         ).toBool();

    trackingStartedAt =
        settings.value(
                    "trackingStartedAt",
                    QDateTime()
                    ).toDateTime();

    if (trackingStartedAt.isValid())
    {
        ui->trackingStartedAtLbl->setText(
            trackingStartedAt.toString("yyyy-MM-dd HH:mm:ss")
            );
    }
    else
    {
        ui->trackingStartedAtLbl->setText("Not started");
    }

    updateCurrentChanceLabel();
}

void MainWindow::writeSettings()
{
    if (ui->devModeCheckBox->isChecked())
    {
        qDebug() << "DEV MODE: writeSettings skipped";
        return;
    }

    QSettings settings;

    settings.setValue(
        "studyNotes",
        ui->studyNotes->toPlainText()
        );

    settings.setValue(
        "lastOpenedTopic",
        lastOpenedTopic
        );

    settings.setValue(
        "chance/startTime",
        chanceStartTime
        );

    settings.setValue(
        "buttonColor/timeSpanMinutes",
        ui->btnColorTimeSpanSpinbox->value()
        );

    settings.setValue(
        "taskIsTriggered",
        taskIsTriggered
        );

    settings.setValue(
        "trackingStartedAt",
        trackingStartedAt
        );

    settings.setValue(
        "studyButtons/progressIsBeingTracked",
        progressIsBeingTracked
        );

    QString activeStudyButtonName;

    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    for (QPushButton *btn : std::as_const(buttons))
    {
        if (!btn->property("trackedColorButton").toBool())
            continue;

        QDateTime lastClicked =
            btn->property("lastClicked").toDateTime();

        QDateTime lastDone =
            btn->property("lastDone").toDateTime();

        int clickCount =
            btn->property("clickCount").toInt();

        qint64 cumulativeSeconds =
            btn->property("cumulativeSeconds").toLongLong();

        bool isPaused =
            btn->property("isPaused").toBool();

        QDateTime pauseStartedAt =
            btn->property("pauseStartedAt").toDateTime();

        qint64 pausedSeconds =
            btn->property("pausedSeconds").toLongLong();

        bool selected =
            btn->property("selected").toBool();

        if (progressIsBeingTracked && selected)
        {
            activeStudyButtonName = btn->objectName();
        }

        QString lastClickedKey =
            QString("studyButtons/%1/lastClicked")
                .arg(btn->objectName());

        QString lastDoneKey =
            QString("studyButtons/%1/lastDone")
                .arg(btn->objectName());

        QString clickCountKey =
            QString("studyButtons/%1/clickCount")
                .arg(btn->objectName());

        QString cumulativeSecondsKey =
            QString("studyButtons/%1/cumulativeSeconds")
                .arg(btn->objectName());

        QString isPausedKey =
            QString("studyButtons/%1/isPaused")
                .arg(btn->objectName());

        QString pauseStartedAtKey =
            QString("studyButtons/%1/pauseStartedAt")
                .arg(btn->objectName());

        QString pausedSecondsKey =
            QString("studyButtons/%1/pausedSeconds")
                .arg(btn->objectName());

        if (lastClicked.isValid())
        {
            settings.setValue(lastClickedKey, lastClicked);
        }
        else
        {
            settings.remove(lastClickedKey);
        }

        if (lastDone.isValid())
        {
            settings.setValue(lastDoneKey, lastDone);
        }
        else
        {
            settings.remove(lastDoneKey);
        }

        settings.setValue(clickCountKey, clickCount);
        settings.setValue(cumulativeSecondsKey, cumulativeSeconds);
        settings.setValue(isPausedKey, isPaused);
        settings.setValue(pausedSecondsKey, pausedSeconds);

        if (pauseStartedAt.isValid())
        {
            settings.setValue(pauseStartedAtKey, pauseStartedAt);
        }
        else
        {
            settings.remove(pauseStartedAtKey);
        }
    }

    if (progressIsBeingTracked && !activeStudyButtonName.isEmpty())
    {
        settings.setValue(
            "studyButtons/activeStudyButtonName",
            activeStudyButtonName
            );
    }
    else
    {
        settings.remove("studyButtons/activeStudyButtonName");
        settings.setValue("studyButtons/progressIsBeingTracked", false);
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
        << "Window with exact title"
        << target
        << "not found.";

    return false;
}

void MainWindow::resetChanceTimer()
{
    chanceStartTime = QDateTime::currentDateTime();
    writeSettings();
}

void MainWindow::onTaskIsDoneBtnClicked()
{
    resetChanceTimer();

    taskIsTriggered = false;

    ui->taskIsDoneBtn->setEnabled(false);
    ui->taskIsDoneBtn->setText("TASK COMPLETED");

    updateCurrentChanceLabel();

    writeSettings();
}

void MainWindow::onReopenLastTopicBtnClicked()
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::onResetTopicsBtnClicked()
{
    QSettings settings;

    if (QMessageBox::question(
            this,
            "Reset topics",
            "Reset all topic statistics and ordering?"
            )
        != QMessageBox::Yes)
    {
        return;
    }

    trackingStartedAt = QDateTime::currentDateTime();

    ui->trackingStartedAtLbl->setText(
        trackingStartedAt.toString("yyyy-MM-dd HH:mm:ss")
        );

    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    for (QPushButton *btn : std::as_const(buttons))
    {
        if (!btn->property("trackedColorButton").toBool())
            continue;

        btn->setProperty("lastClicked", QDateTime());
        btn->setProperty("lastDone", QDateTime());

        btn->setProperty("clickCount", 0);
        btn->setProperty("cumulativeSeconds", 0);
        btn->setProperty("isPaused", false);
        btn->setProperty("pauseStartedAt", QDateTime());
        btn->setProperty("pausedSeconds", 0);

        btn->setProperty("selected", false);

        updateButtonColor(
            btn,
            buttonColorReferenceTime(btn)
            );

        updateButtonStatsLabels(btn);

        settings.remove(
            QString("studyButtons/%1")
                .arg(btn->objectName())
            );
    }

    for (auto it = priorityLayouts.begin();
         it != priorityLayouts.end();
         ++it)
    {
        QVBoxLayout *columnLayout = it.value();

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
                return
                    a->property("originalIndex").toInt()
                    <
                    b->property("originalIndex").toInt();
            }
            );

        for (QPushButton *btn : priorityButtons)
        {
            columnLayout->removeWidget(btn);

            int insertIndex =
                columnLayout->count() - 1;

            if (insertIndex < 0)
                insertIndex = 0;

            columnLayout->insertWidget(
                insertIndex,
                btn
                );
        }
    }

    progressIsBeingTracked = false;

    settings.remove("studyButtons/activeStudyButtonName");
    settings.setValue("studyButtons/progressIsBeingTracked", false);

    writeSettings();
}

// develop

void MainWindow::calculateDogOwnerRating()
{
    qDebug() << "Calculating dog owner rating with:"
             << "amountOfTimesOut:" << m_dogOwnerRating.amountOfTimesOut()
             << "cumulativeTimeOut:" << m_dogOwnerRating.cumulativeTimeOut();

    m_dogOwnerRatingScore =
        m_dogOwnerRating.amountOfTimesOut()
        *
        m_dogOwnerRating.cumulativeTimeOut();

    emit dogOwnerRatingChanged();
}

void MainWindow::amountOfTimesOutChanged(int amountOfTimesOut)
{
    m_dogOwnerRating.setAmountOfTimesOut(amountOfTimesOut);
    calculateDogOwnerRating();
}

void MainWindow::cumulativeTimeOutChanged(int cumulativeTimeOut)
{
    m_dogOwnerRating.setCumulativeTimeOut(cumulativeTimeOut);
    calculateDogOwnerRating();
}

void MainWindow::setupStudyButtons()
{
    QList<StudyButton> buttons = defaultStudyButtons();

    QFontMetrics metrics(ui->productionViewContainer->font());

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

    QWidget *studyButtonsContainer = new QWidget(ui->productionViewContainer);
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
        btn->setProperty("selected", false);
        btn->setProperty("priority", button.priority);
        btn->setProperty("appTitle", button.appTitle);
        btn->setProperty("originalIndex", studyButtonIndex);
        btn->setProperty("clickCount", 0);
        btn->setProperty("cumulativeSeconds", 0);

        updateButtonStatsLabels(btn);

        ++studyButtonIndex;

        priorityLayouts[button.priority]->addWidget(btn);

        connect(
            btn,
            &QPushButton::clicked,
            this,
            [this, btn]()
            {
                handleStudyButtonClicked(btn);
            }
            );
    }

    for (QVBoxLayout *columnLayout : std::as_const(priorityLayouts))
    {
        columnLayout->addStretch();
    }
}

void MainWindow::restoreStudyButtonSettings()
{
    QSettings settings;

    QString activeStudyButtonName =
        settings.value("studyButtons/activeStudyButtonName", "").toString();

    bool restoredActiveStudyButton = false;

    QList<QPushButton*> createdButtons = findChildren<QPushButton*>();

    for (QPushButton *btn : std::as_const(createdButtons))
    {
        if (!btn->property("trackedColorButton").toBool())
            continue;

        QDateTime lastClicked = settings.value(
                                            QString("studyButtons/%1/lastClicked")
                                                .arg(btn->objectName())
                                            ).toDateTime();

        QDateTime lastDone = settings.value(
                                         QString("studyButtons/%1/lastDone")
                                             .arg(btn->objectName())
                                         ).toDateTime();

        int clickCount = settings.value(
                                     QString("studyButtons/%1/clickCount")
                                         .arg(btn->objectName()),
                                     0
                                     ).toInt();

        qint64 cumulativeSeconds = settings.value(
                                               QString("studyButtons/%1/cumulativeSeconds")
                                                   .arg(btn->objectName()),
                                               0
                                               ).toLongLong();

        bool isPaused = settings.value(
                                    QString("studyButtons/%1/isPaused")
                                        .arg(btn->objectName()),
                                    false
                                    ).toBool();

        QDateTime pauseStartedAt = settings.value(
                                               QString("studyButtons/%1/pauseStartedAt")
                                                   .arg(btn->objectName())
                                               ).toDateTime();

        qint64 pausedSeconds = settings.value(
                                           QString("studyButtons/%1/pausedSeconds")
                                               .arg(btn->objectName()),
                                           0
                                           ).toLongLong();

        btn->setProperty("clickCount", clickCount);
        btn->setProperty("cumulativeSeconds", cumulativeSeconds);
        btn->setProperty("isPaused", isPaused);
        btn->setProperty("pauseStartedAt", pauseStartedAt);
        btn->setProperty("pausedSeconds", pausedSeconds);

        bool isActiveButton =
            progressIsBeingTracked
            &&
            !activeStudyButtonName.isEmpty()
            &&
            btn->objectName() == activeStudyButtonName
            &&
            lastClicked.isValid();

        btn->setProperty("selected", isActiveButton);

        if (!isActiveButton)
        {
            btn->setProperty("isPaused", false);
            btn->setProperty("pauseStartedAt", QDateTime());
            btn->setProperty("pausedSeconds", 0);
        }

        if (isActiveButton)
            restoredActiveStudyButton = true;

        if (lastClicked.isValid())
        {
            btn->setProperty("lastClicked", lastClicked);
        }

        if (lastDone.isValid())
        {
            btn->setProperty("lastDone", lastDone);
        }

        updateButtonColor(
            btn,
            buttonColorReferenceTime(btn)
            );

        updateButtonStatsLabels(btn);
    }

    if (progressIsBeingTracked && !restoredActiveStudyButton)
    {
        progressIsBeingTracked = false;
        settings.remove("studyButtons/activeStudyButtonName");
        settings.setValue("studyButtons/progressIsBeingTracked", false);
    }

    for (auto it = priorityLayouts.begin();
         it != priorityLayouts.end();
         ++it)
    {
        QVBoxLayout *columnLayout = it.value();

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
                QDateTime aDone =
                    a->property("lastDone").toDateTime();

                QDateTime bDone =
                    b->property("lastDone").toDateTime();

                if (aDone.isValid() != bDone.isValid())
                    return !aDone.isValid();

                if (aDone.isValid() && bDone.isValid() && aDone != bDone)
                    return aDone < bDone;

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

void MainWindow::handleStudyButtonClicked(QPushButton *btn)
{
    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    QPushButton *previousSelectedButton = nullptr;

    if (progressIsBeingTracked)
    {
        for (QPushButton *otherBtn : std::as_const(buttons))
        {
            if (!otherBtn->property("trackedColorButton").toBool())
                continue;

            if (!otherBtn->property("selected").toBool())
                continue;

            if (otherBtn == btn)
                continue;

            previousSelectedButton = otherBtn;
            break;
        }
    }

    if (previousSelectedButton != nullptr)
    {
        QDateTime lastClicked =
            previousSelectedButton->property("lastClicked").toDateTime();

        if (lastClicked.isValid())
        {
            QDateTime lastDone =
                QDateTime::currentDateTime();

            previousSelectedButton->setProperty("lastDone", lastDone);

            qint64 pausedSeconds =
                previousSelectedButton->property("pausedSeconds").toLongLong();

            bool isPaused =
                previousSelectedButton->property("isPaused").toBool();

            QDateTime pauseStartedAt =
                previousSelectedButton->property("pauseStartedAt").toDateTime();

            if (isPaused && pauseStartedAt.isValid())
            {
                qint64 finalPausedSeconds =
                    pauseStartedAt.secsTo(lastDone);

                if (finalPausedSeconds < 0)
                    finalPausedSeconds = 0;

                pausedSeconds += finalPausedSeconds;

                qDebug() << "Previous topic was auto-ended while paused";
                qDebug() << "Final paused seconds added:" << finalPausedSeconds;
            }

            qint64 elapsedSeconds =
                lastClicked.secsTo(lastDone) - pausedSeconds;

            if (elapsedSeconds < 0)
                elapsedSeconds = 0;

            qDebug() << "Auto-end raw seconds:"
                     << lastClicked.secsTo(lastDone);

            qDebug() << "Auto-end paused seconds excluded:"
                     << pausedSeconds;

            qDebug() << "Auto-end active seconds:"
                     << elapsedSeconds;

            qint64 cumulativeSeconds =
                previousSelectedButton->property("cumulativeSeconds").toLongLong();

            cumulativeSeconds += elapsedSeconds;

            previousSelectedButton->setProperty(
                "cumulativeSeconds",
                cumulativeSeconds
                );

            previousSelectedButton->setProperty("isPaused", false);
            previousSelectedButton->setProperty("pauseStartedAt", QDateTime());
            previousSelectedButton->setProperty("pausedSeconds", 0);

            qint64 elapsedHours =
                elapsedSeconds / 3600;

            qint64 elapsedMinutes =
                (elapsedSeconds % 3600) / 60;

            qint64 remainingSeconds =
                elapsedSeconds % 60;

            QString elapsedText =
                QString("%1h %2m %3s")
                    .arg(elapsedHours)
                    .arg(elapsedMinutes)
                    .arg(remainingSeconds);

            qDebug() << "Previous topic automatically ended:"
                     << previousSelectedButton->objectName();

            qDebug() << "Elapsed between lastClicked and lastDone:"
                     << elapsedText;

            qDebug() << "New cumulative time:"
                     << formatSecondsAsHoursMinutes(cumulativeSeconds);
        }
        else
        {
            qDebug() << "Previous selected topic had no valid lastClicked:"
                     << previousSelectedButton->objectName();
        }

        previousSelectedButton->setProperty("selected", false);
    }

    for (QPushButton *otherBtn : std::as_const(buttons))
    {
        if (!otherBtn->property("trackedColorButton").toBool())
            continue;

        otherBtn->setProperty("selected", false);

        updateButtonColor(
            otherBtn,
            buttonColorReferenceTime(otherBtn)
            );

        updateButtonStatsLabels(otherBtn);
    }

    btn->setProperty("selected", true);

    checkTaskWithChance();

    QDateTime now = QDateTime::currentDateTime();

    btn->setProperty("lastClicked", now);
    btn->setProperty("isPaused", false);
    btn->setProperty("pauseStartedAt", QDateTime());
    btn->setProperty("pausedSeconds", 0);

    progressIsBeingTracked = true;

    qDebug() << "Topic started:"
             << btn->objectName();

    qDebug() << "lastClicked set to:"
             << now.toString("yyyy-MM-dd HH:mm:ss");

    qDebug() << "Pause state reset for new run";

    int clickCount =
        btn->property("clickCount").toInt();

    btn->setProperty("clickCount", clickCount + 1);

    updateButtonColor(btn, buttonColorReferenceTime(btn));
    updateButtonStatsLabels(btn);

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
}
