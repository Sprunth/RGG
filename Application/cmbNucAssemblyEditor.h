#ifndef cmbNucAssemblyEditor_H
#define cmbNucAssemblyEditor_H

#include <QFrame>
//#include <QGraphicsView>

#include <QStringList>

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QGridLayout;
class QFrame;
class cmbNucDragLabel;
class cmbNucAssembly;

class cmbNucAssemblyEditor : public QFrame
{
  Q_OBJECT

public:
  cmbNucAssemblyEditor(QWidget *parent = 0);
  ~cmbNucAssemblyEditor();

  void resetUI(const std::vector<std::vector<std::string> >& Grid,
    QStringList& availableActions);
  void updateLatticeView(int x, int y);
  void clearUI(bool updateUI=true);
  void updateLatticeWithGrid(
    std::vector<std::vector<std::string> >& Grid);

protected:
  void mousePressEvent(QMouseEvent *event);

private:
  //QGraphicsView *GraphicsView;
  // QFrame* LatticeView;
  cmbNucAssembly *CurrentAssembly;

  QGridLayout* LatticeLayout;
  std::vector<std::vector<std::string> > CurrentGrid;
  QStringList ActionList;
};

#endif // cmbNucAssemblyEditor_H
