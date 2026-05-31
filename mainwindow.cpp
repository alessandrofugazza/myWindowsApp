#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Helpers/buttonhelpers.h"

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
#include <random>

struct StudyButton
{
    QString name;
    int priority;
    QString appTitle;
};

// constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
            }
        );

    // hotkey registration

    HWND hwnd = reinterpret_cast<HWND>(winId());

    if (!RegisterHotKey(
            hwnd,
            HOTKEY_ID,
            MOD_SHIFT | MOD_ALT,
            'D'
            ))
    {
        qDebug() << "RegisterHotKey failed. Error:" << GetLastError();
    }
    else
    {
        qDebug() << "RegisterHotKey succeeded";
    }

    // set initial view at launch

    ui->viewsStack->setCurrentWidget(ui->productionView);

    // default current ui state before loading
    // POLISH do this for every setting?
    ui->currentChanceLbl->setText("settings not read yet");

    // load ui state
    readSettings();

    // setting for btn color timespan
    // QUERY why is this here??
    connect(
        ui->btnColorTimeSpanSpinbox,
        QOverload<int>::of(&QSpinBox::valueChanged),
        this,
        [this](int)
        {
            // POLISH can mark the btns here instead next couple lines?
            QList<QPushButton*> buttons = findChildren<QPushButton*>();

            for (QPushButton *btn : std::as_const(buttons))
            {
                if (!btn->property("trackedColorButton").toBool())
                    continue;

                updateButtonColor(
                    btn,
                    btn->property("lastClicked").toDateTime()
                    );

                updateButtonStatsLabels(btn);
            }

            // CHECK am i updating data on each action i can do in UI?
            writeSettings();
        }
        );


    // set up tray icon
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qt-project.org/windows/cursors/images/openhandcursor_32.png"));
    trayIcon->setToolTip("My Windows App");
    trayIcon->show();

    // when tray icon clicked
    connect(
        trayIcon,
        &QSystemTrayIcon::activated,
        this,
        &MainWindow::trayIconActivated
        );

    // change views logic
    // OTHER
    connect(
        ui->actionOtherView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->otherView);
        }
        );

    // PRODUCTION
    connect(
        ui->actionProductionView,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->productionView);
        }
        );

    // OPTIONS
    connect(
        ui->actionEdit_Options,
        &QAction::triggered,
        this,
        [this]()
        {
            ui->viewsStack->setCurrentWidget(ui->optionsView);
        }
        );

    // Study button data list
    // TODO reorder this shit and most of all dont make it hard coded
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
            {"Project G", 4, "Chrome"},
            {"Project P", 5, "Chrome"},
            {"Prompt Engineering", 8, "Chrome"},
            {"Python", 4, "Chrome"},
            {"Swift", 4, "Chrome"},
            {"Web Development", 3, "Chrome"},
            {"Windows Development", 2, "Chrome"},
            {"Netlify", 6, "Netlify"},
            {"Qt Creator", 4, "Qt Creator"},
            {"Chrome Devtools", 5, "Chrome Devtools"},
            {"iOS Development", 2, "Chrome"}
        };

    // measure study btns size so they're all the same
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

    // add padding to btns
    maxButtonWidth += 34;
    maxButtonHeight += 20;

    // TODO make it app wide const
    const int columnGap = 32;

    // create widget containing the dynamically generated btns
    QWidget *studyButtonsContainer = new QWidget(ui->developWidget);
    studyButtonsContainer->setObjectName("studyButtonsContainer");
    // CHECK these values?? where do they come from
    studyButtonsContainer->setGeometry(20, 20, 1720, 580);

    // Creates a horizontal layout inside the container.
    // Each priority column will be added horizontally.
    QHBoxLayout *mainLayout = new QHBoxLayout(studyButtonsContainer);

    // Of horizontal layout, sets spacing, removes margins, and aligns columns to top-left.
    mainLayout->setSpacing(columnGap);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    //Creates a set of unique priority values.
    QSet<int> priorities;

    // Collects all priority numbers from the button list.
    for (const StudyButton &button : buttons)
    {
        priorities.insert(button.priority);
    }

    // Turns the set into a list.
    // QUERY why are we doing this?
    QList<int> sortedPriorities = priorities.values();

    // Sorts priorities from smallest to largest.
    std::sort(sortedPriorities.begin(), sortedPriorities.end());

    // Loops through each priority number.
    for (int priority : sortedPriorities)
    {
        // Creates a widget for one priority column.
        QWidget *columnWidget = new QWidget(studyButtonsContainer);

        // Forces the column width to match the button width.
        columnWidget->setMinimumWidth(maxButtonWidth);
        columnWidget->setMaximumWidth(maxButtonWidth);

        // Creates a vertical layout for this column.
        QVBoxLayout *columnLayout = new QVBoxLayout(columnWidget);

        // Sets vertical spacing, removes margins, and centers items horizontally.
        // TODO spacing should be app wide const
        columnLayout->setSpacing(10);
        columnLayout->setContentsMargins(0, 0, 0, 0);
        columnLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        // Creates the column title label.
        QLabel *title = new QLabel(
            QString("Priority %1").arg(priority),
            columnWidget
            );

        // Makes the label same width as buttons and centers the text.
        title->setMinimumWidth(maxButtonWidth);
        title->setMaximumWidth(maxButtonWidth);
        title->setAlignment(Qt::AlignCenter);

        columnLayout->addWidget(title);

        // Stores the layout in a map.
        priorityLayouts.insert(priority, columnLayout);

        mainLayout->addWidget(columnWidget);
    }

    // Tracks the order of buttons.
    int studyButtonIndex = 0;

    // Loops over every button definition.
    for (const StudyButton &button : buttons)
    {
        // create each, button, inittially under parent studyButtonsContainer
        QPushButton *btn = new QPushButton(
            formatButtonText(button.name),
            studyButtonsContainer
            );

        // set to weight semibold
        QFont font = btn->font();
        font.setWeight(QFont::DemiBold);
        btn->setFont(font);

        // QUERY why not convert to camelcase for consitency
        btn->setObjectName(button.name);

        btn->setMinimumSize(maxButtonWidth, maxButtonHeight);
        btn->setMaximumSize(maxButtonWidth, maxButtonHeight);

        // Tells layouts not to stretch or shrink the button.
        // QUERY why is this needed?
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

        // QUERY why is this increment here? and not at start
        ++studyButtonIndex;

        priorityLayouts[button.priority]->addWidget(btn);

        // LOGIC FOR WHEN A STUDY BTN IS CLICKED
        connect(
            btn,
            &QPushButton::clicked,
            this,
            [this, btn]()
            {
                QList<QPushButton*> buttons = findChildren<QPushButton*>();

                for (QPushButton *otherBtn : std::as_const(buttons))
                {
                    if (!otherBtn->property("trackedColorButton").toBool())
                        continue;

                    otherBtn->setProperty("selected", false);

                    updateButtonColor(
                        otherBtn,
                        otherBtn->property("lastClicked").toDateTime()
                        );

                    updateButtonStatsLabels(otherBtn);
                }

                btn->setProperty("selected", true);

                checkTaskWithChance();

                QDateTime now = QDateTime::currentDateTime();

                // QUERY do all this after it check wether click was valid?
                btn->setProperty("lastClicked", now);

                progressIsBeingTracked = true;

                int clickCount =
                    btn->property("clickCount").toInt();

                btn->setProperty("clickCount", clickCount + 1);

                updateButtonColor(btn, now);
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
                    // TODO other apps
                    if (appTitle == "Chrome")
                    {
                        activateWindowByTitle(instanceName);
                    }
                }
            }
            );
    }

    // Adds empty flexible space at the bottom of every priority column.
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

            btn->setProperty("clickCount", clickCount);
            btn->setProperty("cumulativeSeconds", cumulativeSeconds);

            if (lastClicked.isValid())
            {
                btn->setProperty("lastClicked", lastClicked);

                updateButtonColor(btn, lastClicked);
            }

            if (lastDone.isValid())
            {
                btn->setProperty("lastDone", lastDone);
            }

            updateButtonStatsLabels(btn);
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

    // Set pointer cursor on all buttons
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
        HOTKEY_ID
        );

    delete ui;
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

    if (msg->message == WM_HOTKEY)
    {
        qDebug() << "WM_HOTKEY received";

        if (msg->wParam == HOTKEY_ID)
        {
            qDebug() << "Shift + Alt + D pressed globally";

            // ALL THIS IS FOR REFOCUS

            // if (isMinimized())
            //     showNormal();

            // show();
            // raise();
            // activateWindow();

            // ui->viewsStack->setCurrentWidget(ui->developWidget);

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

            qint64 elapsedSeconds =
                lastClicked.secsTo(lastDone);

            if (elapsedSeconds < 0)
                elapsedSeconds = 0;

            qint64 cumulativeSeconds =
                selectedButton->property("cumulativeSeconds").toLongLong();

            cumulativeSeconds += elapsedSeconds;

            selectedButton->setProperty("cumulativeSeconds", cumulativeSeconds);

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

            updateButtonStatsLabels(selectedButton);

            // DEBUG

            progressIsBeingTracked = false;

            writeSettings();

            QMessageBox::information(
                this,
                "Last done saved",
                QString(
                    "Topic: %1\n"
                    "Last opened: %2\n"
                    "Last done: %3\n"
                    "Elapsed: %4\n"
                    "Total: %5"
                    )
                    .arg(selectedButton->objectName())
                    .arg(lastClicked.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(lastDone.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(elapsedText)
                    .arg(formatSecondsAsHoursMinutes(cumulativeSeconds))
                );

            qDebug() << "Global hotkey pressed";
            qDebug() << "Last done saved for" << selectedButton->objectName();
            qDebug() << "Elapsed between lastClicked and lastDone:" << elapsedText;

            return true;
        }
    }

    // If it was not your hotkey, let the normal Qt event handler process it.
    return QMainWindow::nativeEvent(
        eventType,
        message,
        result
        );
}

// actual functions

// get current chance of task succeeding
double MainWindow::calculateCurrentTaskChance() const
{
    if (!chanceStartTime.isValid())
        return 0.0;

    // POLISH app wide const?
    constexpr double maxSeconds = 25.0 * 60.0;

    double elapsedSeconds =
        chanceStartTime.secsTo(QDateTime::currentDateTime());

    double progress = elapsedSeconds / maxSeconds;

    progress = qBound(0.0, progress, 1.0);

    return progress;
}

// check if task is triggered against currentchance
void MainWindow::checkTaskWithChance()
{
    double chance = calculateCurrentTaskChance();

    QDateTime now = QDateTime::currentDateTime();

    // QUERY why not .secsTo?
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

// runs when checkTaskWithChance succeeds
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

// Updates the chance label and progress bar.
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

    // POLISH comparing floating-point with == is often not ideal, but here currentChance() clamps to exactly 1.0, so it is acceptable.
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

    // TODO better interpolation
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
    // Runs when the window becomes active/focused.
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

            updateButtonStatsLabels(btn);
        }

        updateCurrentChanceLabel();
    }

    return QMainWindow::event(event);
}

// TODO make this more programmatic
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

    // TODO app wide const for default value
    ui->btnColorTimeSpanSpinbox->setValue(
        settings.value("buttonColor/timeSpanMinutes", 120).toInt()
        );

    if (!chanceStartTime.isValid())
        chanceStartTime = QDateTime::currentDateTime();

    // TODO Potential issue: if taskIsTriggered is true, this function returns immediately, so it will not restore the button text to "TASK COMPLETED" or "MOVE TOPIC". It only remembers that something was triggered, not which one.
    taskIsTriggered = settings.value(
                                  "taskIsTriggered",
                                  false
                                  ).toBool();

    updateCurrentChanceLabel();
}

// TODO better default values
void MainWindow::writeSettings()
{
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

    // save study btns lastclick

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
    }
}

// Searches open Windows windows by exact title.
//If it finds one, it activates it.

bool MainWindow::activateWindowByTitle(const QString &target)
{
    // Gets the first top-level window.
    HWND hwnd = FindWindowA(nullptr, nullptr);

    // Loops through windows until there are no more.
    // CHECK any way to avoid looping over everything?
    while (hwnd != nullptr)
    {
        char title[512];

        // POLISH potential issue: this is not Unicode-safe. For non-ASCII window titles, GetWindowTextW would be better.
        GetWindowTextA(hwnd, title, sizeof(title));

        if (title[0] != '\0')
        {
            QString t = QString::fromLatin1(title);

            if (t == target)
            {
                ShowWindow(hwnd, SW_MAXIMIZE);
                SetForegroundWindow(hwnd);

                // CHECK this lasttopic position only makes sense for chrome istances
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

    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    for (QPushButton *btn : std::as_const(buttons))
    {
        if (!btn->property("trackedColorButton").toBool())
            continue;

        btn->setProperty("lastClicked", QDateTime());
        btn->setProperty("lastDone", QDateTime());

        btn->setProperty("clickCount", 0);
        btn->setProperty("cumulativeSeconds", 0);

        btn->setProperty("selected", false);

        updateButtonColor(
            btn,
            QDateTime()
            );

        updateButtonStatsLabels(btn);

        settings.remove(
            QString("studyButtons/%1")
                .arg(btn->objectName())
            );
    }

    //
    // restore original ordering
    //

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

    writeSettings();
}