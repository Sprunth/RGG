#ifndef cmbNucNewDialog_H
#define cmbNucNewDialog_H

#include <QDialog>

// Forward Qt class declarations
class Ui_qNucNewDialog;
class cmbNucMainWindow;

class cmbNucNewDialog : public QDialog
{
  Q_OBJECT
public:
  cmbNucNewDialog(cmbNucMainWindow* mainWindow);
  ~cmbNucNewDialog();

  enum CoreGeometry {
    RECTILINEAR,
    HEXAGONAL
  };

  CoreGeometry getSelectedGeometry();

private:
  // Designer form
  Ui_qNucNewDialog *ui;
};

#endif
