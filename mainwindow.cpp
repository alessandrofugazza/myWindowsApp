#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QString>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <utility>
#include <QSet>
#include <algorithm>

#include <windows.h>

struct StudyButton
{
    QString name;
    int priority;
    QString appTitle;
};

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

    connect(ui->actionProductionView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentIndex(0);
            });

    connect(ui->actionDevelopView, &QAction::triggered, this, [this]()
            {
                ui->viewsStack->setCurrentIndex(1);
            });

    // study buttons

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
            {"Doblo", 2, ""},
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

    QHBoxLayout *mainLayout = new QHBoxLayout(ui->developWidget);
    ui->developWidget->setLayout(mainLayout);

    QSet<int> priorities;

    for (const StudyButton &button : buttons)
    {
        priorities.insert(button.priority);
    }

    QList<int> sortedPriorities = priorities.values();

    std::sort(sortedPriorities.begin(), sortedPriorities.end());

    for (int priority : sortedPriorities)
    {
        QVBoxLayout *columnLayout = new QVBoxLayout();

        QLabel *title = new QLabel(
            QString("Priority %1").arg(priority),
            ui->developWidget
            );

        title->setAlignment(Qt::AlignCenter);

        columnLayout->addWidget(title);

        priorityLayouts.insert(priority, columnLayout);

        mainLayout->addLayout(columnLayout);
    }

    for (const StudyButton &button : buttons)
    {
        QPushButton *btn = new QPushButton(button.name, ui->developWidget);

        btn->setObjectName(button.name);

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

                    QVBoxLayout *columnLayout = priorityLayouts.value(priority, nullptr);

                    if (columnLayout != nullptr)
                    {
                        columnLayout->removeWidget(btn);

                        int insertIndex = columnLayout->count() - 1;

                        if (insertIndex < 0)
                            insertIndex = 0;

                        columnLayout->insertWidget(insertIndex, btn);
                    }

                    QString appTitle = btn->property("appTitle").toString();
                    QString instanceName = btn->objectName();

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

    mainLayout->addStretch();

    // timer

    connect(&timer, &QTimer::timeout,
            this, &MainWindow::updateCountdown);

    // read settings

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

    int r = static_cast<int>(255 * t);

    int g = static_cast<int>(255 * (1.0 - t));

    btn->setStyleSheet(
        QString(
            "background-color: rgb(%1,%2,0);"
            )
            .arg(r)
            .arg(g)
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
}

void MainWindow::writeSettings()
{
    QSettings settings;

    settings.setValue(
        "notConcluded",
        ui->notConcluded->toPlainText()
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
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);

                qDebug() << "Window with title containing" << target << "found.";

                return true;
            }
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    qDebug() << "Window with title containing" << target << "not found.";

    return false;
}

void MainWindow::handleWindowButton(QPushButton *btn, const QString &target)
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
        ui->timeLeftLabel->setText(QString::number(remainingTime));

    if (remainingTime <= 0)
    {
        timer.stop();

        trayIcon->showMessage(
            "Timer finished",
            "Your countdown ended.",
            QSystemTrayIcon::Information,
            5000
            );
    }
}

void MainWindow::on_startTimerBtn_clicked()
{
    remainingTime = 5;

    ui->timeLeftLabel->setText(QString::number(remainingTime));

    timer.start(1000);
}