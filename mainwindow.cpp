#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QAction>
#include <QDebug>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QString>
#include <QRandomGenerator>
#include <QSettings>
#include <windows.h>
#include <QCloseEvent>
#include <QFile>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , remainingTime(0)
{
    ui->setupUi(this);

    // Create tray icon
    // QIcon icon(":/img/icons/sources/img/icons/icon.ico");

    // qDebug() << "icon is null:" << icon.isNull();

    // QFile file(":/img/icons/sources/img/icons/icon.ico");
    // qDebug() << file.exists();

    // qDebug() << QDir(":/").entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst);


    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/img/icons/sources/img/icons/icon.ico"));

    trayIcon->setToolTip("My Windows App");
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::trayIconActivated);

    connect(ui->actionProductionView, &QAction::triggered, this, [this]{
        ui->viewsStack->setCurrentIndex(0);
    });

    connect(ui->actionDevelopView, &QAction::triggered, this, [this]{
        ui->viewsStack->setCurrentIndex(1);
    });







    connect(ui->startTimerBtn, &QPushButton::clicked,
            this, &MainWindow::startCountdown);

    connect(&timer, &QTimer::timeout,
            this, &MainWindow::updateCountdown);

    readSettings();
}

MainWindow::~MainWindow()
{
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings(); // save data just before closing
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


bool MainWindow::event(QEvent *event)
{
    // if (event->type() == QEvent::WindowActivate) {
    //     // Window gained focus → update button color
    //     ui->openMainBtn->setStyleSheet("background-color: red;");
    // }

    return QWidget::event(event);
}

bool MainWindow::activateWindowByTitle(const QString &target)
{
    HWND hwnd = FindWindowA(nullptr, nullptr);

    while (hwnd != nullptr) {
        char title[512];
        GetWindowTextA(hwnd, title, sizeof(title));

        if (title[0] != '\0') {
            QString t = QString::fromLatin1(title);
            if (t.contains(target, Qt::CaseInsensitive)) {
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
    // Color for "clicked"
    // btn->setStyleSheet("background-color: #4CAF50;");

    // Try to activate the external window
    activateWindowByTitle(target);
}


// void MainWindow::on_openMainBtn_clicked()
// {
//     handleWindowButton(ui->openMainBtn, "main");
// }

void MainWindow::on_openLeftBtn_clicked()
{
    handleWindowButton(ui->openLeftBtn, "left");
}

// void MainWindow::on_openRightBtn_clicked()
// {
//     handleWindowButton(ui->openRightBtn, "right");
// }

// void MainWindow::on_openWebtestBtn_clicked()
// {
//     handleWindowButton(ui->openWebtestBtn, "webtest");
// }

// void MainWindow::on_openRfcBtn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "reference");
// }

// void MainWindow::on_openTools1Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Tools 1");
// }

// void MainWindow::on_openTools2Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Tools 2");
// }

// void MainWindow::on_openTools3Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Tools 3");
// }

// void MainWindow::on_openDeepen1Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Deepen 1");
// }

// void MainWindow::on_openDeepen2Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Deepen 2");
// }

// void MainWindow::on_openDeepen3Btn_clicked()
// {
//     handleWindowButton(ui->openRfcBtn, "Deepen 3");
// }

void MainWindow::startCountdown()
{
    remainingTime = 5;
    ui->timeLeftLabel->setText(QString::number(remainingTime));
    timer.start(1000); // 1 second
}

void MainWindow::updateCountdown()
{
    remainingTime--;

    if (remainingTime >= 0)
        ui->timeLeftLabel->setText(QString::number(remainingTime));

    if (remainingTime <= 0) {
        timer.stop();
        trayIcon->showMessage(
            "Timer finished",
            "Your countdown ended.",
            QSystemTrayIcon::Information,
            5000
            );
    }

}

