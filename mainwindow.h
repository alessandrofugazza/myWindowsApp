#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QMap>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QRandomGenerator>

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
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void on_activateMainBtn_clicked();
    void on_activateLeftBtn_clicked();
    void on_activateRightBtn_clicked();

    void on_activateWebtestRightBtn_clicked();
    void on_activateWebtestLeftBtn_clicked();
    void on_activateRfcBtn_clicked();



    void on_taskIsDoneBtn_clicked();

    void on_reopenLastTopicBtn_clicked();



    // develop



private:
    Ui::MainWindow *ui;

    QTimer timer;
    int remainingTime;

    QSystemTrayIcon *trayIcon = nullptr;

    QMap<int, QVBoxLayout*> priorityLayouts;

    bool activateWindowByTitle(const QString &target);
    void handleWindowButton(QPushButton *btn, const QString &target);

    void readSettings();
    void writeSettings();

    void updateButtonColor(QPushButton *btn, QDateTime clickedTime);

    QElapsedTimer chanceTimer;
    QDateTime chanceStartTime;
    QDateTime moveTopicChanceStartTime;

    void resetMoveTopicChanceTimer();
    double currentMoveTopicChance() const;
    void updateMoveTopicChanceLabel();


    double currentChance() const;
    void resetChanceTimer();
    void checkTaskWithChance();
    void updateCurrentChanceLabel();
    void doTaskTriggeredStuff();
    QString lastOpenedTopic;
    bool taskIsTriggered = false;
};

#endif // MAINWINDOW_H