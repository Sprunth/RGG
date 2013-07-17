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

  void setLattice(Lattice *lattice);
  void resetUI();
  void updateLatticeView(int x, int y);
  void clearUI(bool updateUI=true);
  void updateLatticeWithGrid(Lattice* lattice);

signals:
  void pinMoved();

protected:
  void mousePressEvent(QMouseEvent *event);

private:
  //QGraphicsView *GraphicsView;
  // QFrame* LatticeView;
  Lattice *CurrentLattice;

  const QRect targetRect(const QPoint &position) const;
  int pieceWidth() const;
  int pieceHeight() const;
  cmbNucDragLabel* findLabel(const QRect &pieceRect);

  //QList<QPixmap> piecePinCells;
  //QList<QRect> pieceRects;
  //QList<QPoint> pieceLocations;
  QGridLayout* LatticeLayout;
  std::vector<std::vector<std::string> > CurrentGrid;

};

#endif // cmbNucAssemblyEditor_H
