#include "cmbNucDragLabel.h"

#include <QtGui>

cmbNucDragLabel::cmbNucDragLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setAutoFillBackground(true);
    setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Raised);
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
}
