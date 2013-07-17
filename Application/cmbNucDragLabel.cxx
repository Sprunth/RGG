#include "cmbNucDragLabel.h"

#include <QtGui>

cmbNucDragLabel::cmbNucDragLabel(const QString &text, QWidget *parent,
  int i, int j) : QLabel(text, parent), x(i), y(j)
{
    setAutoFillBackground(true);
    setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Raised);
    setAcceptDrops(true);
    setAlignment(Qt::AlignCenter);
    QFont font = this->font();
    font.setBold(true);
    this->setFont(font);

    highlighted = false;
}

void cmbNucDragLabel::paintEvent(QPaintEvent *event)
{
  this->QLabel::paintEvent(event);
  QPalette palette = this->palette();
  if (highlighted)
    {
    palette.setColor(this->backgroundRole(), QColor("#ffcccc"));
    }
  else
    {
    palette.setColor(this->backgroundRole(), Qt::gray);
    }
  this->setPalette(palette);
}

void cmbNucDragLabel::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasText()){
    this->setHighlight(true);
    update();
    event->setDropAction(Qt::CopyAction);
    event->accept();
    }
  else
    event->ignore();
}

void cmbNucDragLabel::dragLeaveEvent(QDragLeaveEvent *event)
{
    this->setHighlight(false);
    update();
    event->accept();
}

void cmbNucDragLabel::dropEvent(QDropEvent *event)
{
   if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        QString piecetext = mime->text();
        this->setText(piecetext);
        this->setHighlight(false);
        update();
        event->accept();
    } else {
        event->ignore();
    }
}
