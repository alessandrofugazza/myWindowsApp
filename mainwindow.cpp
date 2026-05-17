#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QString>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <utility>

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
            {"Task A", 1, "main"},
            {"Task B", 1, "left"},
            {"Task C", 2, "right"},
            {"Task D", 2, ""},
            {"Task E", 3, "webtest"}
        };

    QHBoxLayout *mainLayout = new QHBoxLayout(ui->developWidget);
    ui->developWidget->setLayout(mainLayout);

    QMap<int, QVBoxLayout*> priorityLayouts;

    for (const StudyButton &button : buttons)
    {
        if (!priorityLayouts.contains(button.priority))
        {
            QVBoxLayout *columnLayout = new QVBoxLayout();

            QLabel *title = new QLabel(
                QString("Priority %1").arg(button.priority),
                ui->developWidget
                );

            title->setAlignment(Qt::AlignCenter);
            columnLayout->addWidget(title);

            priorityLayouts.insert(button.priority, columnLayout);
            mainLayout->addLayout(columnLayout);
        }

        QPushButton *btn = new QPushButton(button.name, ui->developWidget);

        btn->setProperty("trackedColorButton", true);
        btn->setProperty("priority", button.priority);
        btn->setProperty("appTitle", button.appTitle);

        priorityLayouts[button.priority]->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, btn]()
                {
                    QDateTime now = QDateTime::currentDateTime();

                    btn->setProperty("lastClicked", now);

                    updateButtonColor(btn, now);

                    QString appTitle = btn->property("appTitle").toString();

                    if (!appTitle.isEmpty())
                    {
                        activateWindowByTitle(appTitle);
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

    qint64 seconds = clickedTime.secsTo(QDateTime::currentDateTime());

    int r;
    int g;

    if (seconds < 30)
    {
        double t = seconds / 30.0;

        r = static_cast<int>(255 * t);
        g = 255;
    }
    else
    {
        double t = qMin((seconds - 30) / 30.0, 1.0);

        r = 255;
        g = static_cast<int>(255 * (1.0 - t));
    }

    btn->setStyleSheet(
        QString("background-color: rgb(%1,%2,0);")
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