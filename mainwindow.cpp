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

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/qt-project.org/windows/cursors/images/openhandcursor_32.png"));
    trayIcon->setToolTip("My Windows App");
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::trayIconActivated);

    connect(ui->actionOtherView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentIndex(0);
            });

    connect(ui->actionProductionView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentIndex(1);
            });

    QList<StudyButton> buttons =
        {
            {"AHK", 4, "Chrome"},
            {"Anki", 3, "Chrome"},
            {"Artificial Intelligence", 6, "Chrome"},
            {"Automotive", 2, "Chrome"},
            {"C++", 4, "Chrome"},
            {"CB125R", 1, ""},
            {"CRI", 5, "Chrome"},
            {"Canophilia", 2, "Chrome"},
            {"Chrome Extensions Development", 2, "Chrome"},
            {"CompTIA", 1, "Chrome"},
            {"Data Science", 6, "Chrome"},
            {"Deepen 1", 4, "Chrome"},
            {"Deepen 2", 5, "Chrome"},
            {"Deepen 3", 6, "Chrome"},
            {"Doblo", 1, ""},
            {"Excel", 3, "Chrome"},
            {"FTO", 1, "Chrome"},
            {"Finance", 1, "Chrome"},
            {"Flipper", 7, "Chrome"},
            {"GitHub Copilot", 1, "Chrome"},
            {"JavaScript", 4, "Chrome"},
            {"Law", 3, ""},
            {"Learn", 2, "Chrome"},
            {"Markdown", 1, "Chrome"},
            {"Obsidian", 1, "Chrome"},
            {"PLC", 5, "Chrome"},
            {"Power User", 5, "Chrome"},
            {"Project Serena", 3, "Chrome"},
            {"Prompt Engineering", 9, "Chrome"},
            {"Python", 4, "Chrome"},
            {"Swift", 4, "Chrome"},
            {"Tools 1", 3, "Chrome"},
            {"Tools 2", 4, "Chrome"},
            {"Tools 3", 5, "Chrome"},
            {"Web Development", 3, "Chrome"},
            {"Windows Development", 2, "Chrome"},
            {"Word", 6, "Chrome"},
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

    QHBoxLayout *mainLayout = new QHBoxLayout(ui->developWidget);
    ui->developWidget->setLayout(mainLayout);

    mainLayout->setSpacing(columnGap);
    mainLayout->setContentsMargins(20, 20, 20, 20);
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
        QWidget *columnWidget = new QWidget(ui->developWidget);

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

    for (const StudyButton &button : buttons)
    {
        QPushButton *btn = new QPushButton(
            formatButtonText(button.name),
            ui->developWidget
            );

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

        priorityLayouts[button.priority]->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, btn]()
                {
                    QDateTime now = QDateTime::currentDateTime();

                    btn->setProperty("lastClicked", now);

                    updateButtonColor(btn, now);

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

    connect(&timer, &QTimer::timeout,
            this, &MainWindow::updateCountdown);

    readSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
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

    constexpr double maxSeconds = 60.0 * 60.0;

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

            if (t.contains(target, Qt::CaseInsensitive))
            {
                ShowWindow(hwnd, SW_MAXIMIZE);
                SetForegroundWindow(hwnd);

                qDebug()
                    << "Window with title containing"
                    << target
                    << "found.";

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