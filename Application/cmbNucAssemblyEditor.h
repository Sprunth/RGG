#ifndef cmbNucAssemblyEditor_H
#define cmbNucAssemblyEditor_H

#include <QFrame>
//#include <QGraphicsView>

#include <QStringList>
#include "cmbNucPartDefinition.h"

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QGridLayout;
class QFrame;
class cmbNucDragLabel;
class cmbNucAssembly;
class LatticeCell;

class cmbNucAssemblyEditor : public QFrame
{
  Q_OBJECT

public:
  cmbNucAssemblyEditor(QWidget *parent, cmbNucAssembly* assy);
  ~cmbNucAssemblyEditor();

  void resetUI(const std::vector<std::vector<LatticeCell> >& Grid,
    QStringList& availableActions);
  void updateLatticeView(int x, int y);
  void clearUI(bool updateUI=true);
  void updateLatticeWithGrid(
    std::vector<std::vector<LatticeCell> >& Grid);

  void setAssembly(cmbNucAssembly* assy);

protected:
  void mousePressEvent(QMouseEvent *event);

private:
  cmbNucAssembly* CurrentAssembly;

  QGridLayout* LatticeLayout;
  std::vector<std::vector<LatticeCell> > CurrentGrid;
  QStringList ActionList;
};

#endif // cmbNucAssemblyEditor_H
