#ifndef cmbNucAssemblyEditor_H
#define cmbNucAssemblyEditor_H

#include <QWidget>
#include <QGraphicsView>

#include "cmbNucAssembly.h"

class cmbNucAssemblyEditor : public QWidget
{
  Q_OBJECT

public:
  cmbNucAssemblyEditor(QWidget *parent = 0);
  ~cmbNucAssemblyEditor();

  void SetAssembly(cmbNucAssembly *assembly);

private:
  QGraphicsView *GraphicsView;
  cmbNucAssembly *Assembly;
};

#endif // cmbNucAssemblyEditor_H
