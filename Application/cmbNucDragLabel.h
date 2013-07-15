
#ifndef cmbNucDragLabel_H
#define cmbNucDragLabel_H

#include <QLabel>

class QDragEnterEvent;
class QDragMoveEvent;
class QFrame;

class cmbNucDragLabel : public QLabel
{
public:
    cmbNucDragLabel(const QString &text, QWidget *parent);
};

#endif
