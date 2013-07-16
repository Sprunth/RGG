
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
    //set/get highlight 
    void setHighlight(bool val){this->highlighted=val;}
    bool getHighlight(){return this->highlighted;}

protected:
  void paintEvent(QPaintEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);

private:
  bool highlighted;
};

#endif
