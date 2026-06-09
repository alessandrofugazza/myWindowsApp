#include "rockwidget.h"
#include <QMessageBox>
#include <QLabel>
#include <QDebug>

RockWidget::RockWidget(QWidget *parent)
    : QWidget{parent}
{
    setStyleSheet("background-color: red;");

    QLabel *label = new QLabel("This is the Rock Widget", this);

    // qDebug("RockWidget created");
}

void RockWidget::buttonClicked()
{
    QMessageBox::information(
        this,
        "Rock button clicked",
        "You clicked the rock button!"
        );
}

QSize RockWidget::sizeHint() const
{
    return QSize(500, 500);
}
