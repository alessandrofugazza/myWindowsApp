#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;



protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:



    void updateCountdown();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_activateMainBtn_clicked();
    void on_activateLeftBtn_clicked();
    void on_activateRightBtn_clicked();

    void on_activateWebtestRightBtn_clicked();
    void on_activateWebtestLeftBtn_clicked();
    void on_activateRfcBtn_clicked();

    // void on_openTools1Btn_clicked();
    // void on_openTools2Btn_clicked();
    // void on_openTools3Btn_clicked();

    // void on_openDeepen1Btn_clicked();
    // void on_openDeepen2Btn_clicked();
    // void on_openDeepen3Btn_clicked();

    void on_startTimerBtn_clicked();

private:
    Ui::MainWindow *ui;
    QTimer timer;
    int remainingTime;

    QSystemTrayIcon *trayIcon = nullptr;

    bool activateWindowByTitle(const QString &target);
    void handleWindowButton(QPushButton *btn, const QString &target);


    void readSettings();
    void writeSettings();
};
#endif // MAINWINDOW_H
