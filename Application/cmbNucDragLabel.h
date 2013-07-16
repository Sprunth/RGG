
#ifndef __cmbNucDragLabel_H
#define __cmbNucDragLabel_H

#include <QLabel>

class QDragEnterEvent;
class QDragMoveEvent;
class QFrame;

class cmbNucDragLabel : public QLabel
{
  Q_OBJECT

public:
    cmbNucDragLabel(const QString &text, QWidget *parent);

};

#endif
