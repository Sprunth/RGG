
#ifndef __cmbNucDragLabel_H
#define __cmbNucDragLabel_H

#include <QLabel>
#include <QColor>
#include <QStringList>
#include <QWeakPointer>

class QDragEnterEvent;
class QDragMoveEvent;
class QFrame;

class cmbNucDragLabel : public QLabel
{
  Q_OBJECT

public:
  cmbNucDragLabel(const QString &text, QWidget *parent, int i, int j);
  //set/get highlight
  void setHighlight(bool val){this->highlighted=val;}
  bool getHighlight(){return this->highlighted;}

  // postion in the grid
  int getX(){return x;}
  int getY(){return y;}

  void setBackgroundColor(const QColor& color);
  QColor getBackgroundColor();

protected:
  void paintEvent(QPaintEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dropEvent(QDropEvent *event);

private:
  bool highlighted;
  // position in the grid
  int x, y;
  QColor backgroundColor;
};

#endif
