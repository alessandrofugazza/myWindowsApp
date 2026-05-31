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
    bool nativeEvent(
        const QByteArray &eventType,
        void *message,
        qintptr *result
        ) override;


private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTaskIsDoneBtnClicked();
    void onReopenLastTopicBtnClicked();
    void onResetTopicsBtnClicked();

private:
    Ui::MainWindow *ui;

    QTimer timer;

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


    double calculateCurrentTaskChance() const;
    void resetChanceTimer();
    void checkTaskWithChance();
    void updateCurrentChanceLabel();
    void doTaskTriggeredStuff();
    QString lastOpenedTopic;
    bool taskIsTriggered = false;

    static constexpr int HOTKEY_ID = 1;

    QPushButton *selectedStudyButton = nullptr;

    bool progressIsBeingTracked = false;

    void setupStudyButtons();

    void restoreStudyButtonSettings();
};

#endif // MAINWINDOW_H