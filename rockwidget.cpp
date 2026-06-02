#include "rockwidget.h"
#include <QMessageBox>

RockWidget::RockWidget(QWidget *parent)
    : QWidget{parent}
{}

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
