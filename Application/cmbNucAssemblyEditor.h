#ifndef cmbNucAssemblyEditor_H
#define cmbNucAssemblyEditor_H

#include <QWidget>
//#include <QGraphicsView>

#include "cmbNucAssembly.h"

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QGridLayout;
class QFrame;

class cmbNucAssemblyEditor : public QWidget
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
  void dragEnterEvent(QDragEnterEvent *event);
  void dragLeaveEvent(QDragLeaveEvent *event);
  void dragMoveEvent(QDragMoveEvent *event);
  void dropEvent(QDropEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void paintEvent(QPaintEvent *event);

private:
  //QGraphicsView *GraphicsView;
  QFrame* LatticeView;
  cmbNucAssembly *Assembly;

  int findPiece(const QRect &pieceRect) const;
  const QRect targetSquare(const QPoint &position) const;
  int pieceSize() const;
  void clear(bool updateUI=true);

  QList<QPixmap> piecePinCells;
  QList<QRect> pieceRects;
  QList<QPoint> pieceLocations;
  QRect highlightedRect;
  QGridLayout* LatticeLayout;

};

#endif // cmbNucAssemblyEditor_H
