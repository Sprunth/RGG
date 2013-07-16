#ifndef cmbNucAssemblyEditor_H
#define cmbNucAssemblyEditor_H

#include <QFrame>
//#include <QGraphicsView>

#include "cmbNucAssembly.h"

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QGridLayout;
class QFrame;
class cmbNucDragLabel;

class cmbNucAssemblyEditor : public QFrame
{
  Q_OBJECT

public:
  cmbNucAssemblyEditor(QWidget *parent = 0);
  ~cmbNucAssemblyEditor();

  void setAssembly(cmbNucAssembly *assembly);
  void resetUI();

signals:
  void pinMoved();

protected:

  void mousePressEvent(QMouseEvent *event);

private:
  //QGraphicsView *GraphicsView;
  // QFrame* LatticeView;
  cmbNucAssembly *Assembly;

  const QRect targetRect(const QPoint &position) const;
  int pieceWidth() const;
  int pieceHeight() const;
  cmbNucDragLabel* findLabel(const QRect &pieceRect);
  void clear(bool updateUI=true);

  //QList<QPixmap> piecePinCells;
  //QList<QRect> pieceRects;
  //QList<QPoint> pieceLocations;
  QGridLayout* LatticeLayout;

};

#endif // cmbNucAssemblyEditor_H
