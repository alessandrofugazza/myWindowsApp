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

// wraps long button text onto multiple lines
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

// constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


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
        btn->setProperty("priority", button.priority);
        btn->setProperty("appTitle", button.appTitle);
        btn->setProperty("originalIndex", studyButtonIndex);

        // QUERY why is this increment here? and not at start
        ++studyButtonIndex;

        priorityLayouts[button.priority]->addWidget(btn);

        connect(
            btn,
            &QPushButton::clicked,
            this,
            [this, btn]()
            {
                checkTaskWithChance();

                QDateTime now = QDateTime::currentDateTime();

                // QUERY do all this after it check wether click was valid?
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
                    // TODO other apps
                    if (appTitle == "Chrome")
                    {
                        activateWindowByTitle(instanceName);
                    }
                }
            }
            );
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

    QList<QPushButton*> allButtons = findChildren<QPushButton*>();

    for (QPushButton *btn : allButtons)
    {
        btn->setCursor(Qt::PointingHandCursor);
    }

    ui->taskIsDoneBtn->raise();
    ui->reopenLastTopicBtn->raise();
}

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

            if (isMinimized())
                showNormal();

            show();
            raise();
            activateWindow();

            ui->viewsStack->setCurrentWidget(ui->developWidget);
            ui->currentChanceLbl->setText("Global hotkey pressed");

            return true;
        }
    }

    return QMainWindow::nativeEvent(
        eventType,
        message,
        result
        );
}

void MainWindow::checkTaskWithChance()
{
    double chance = currentChance();

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
            "border: 1px solid rgba(255,255,255,80);"
            "border-radius: 4px;"
            "padding: 4px 8px;"
            "}"
            )
            .arg(r)
            .arg(g)
            .arg(b)
        );
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
                btn->property("lastClicked").toDateTime()
                );
        }

        updateCurrentChanceLabel();
    }

    return QMainWindow::event(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    QWidget::closeEvent(event);
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

    updateCurrentChanceLabel();
}

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
        << "Window with exact title"
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

void MainWindow::resetChanceTimer()
{
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

    taskIsTriggered = false;

    ui->taskIsDoneBtn->setEnabled(false);
    ui->taskIsDoneBtn->setText("TASK IS DONE");

    updateCurrentChanceLabel();

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