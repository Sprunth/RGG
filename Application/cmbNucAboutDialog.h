#ifndef __cmbNucAboutDialog_h
#define __cmbNucAboutDialog_h

#include <QDialog>

namespace Ui { class AboutDialog; }

class QPixmap;

/// Provides an about dialog
class cmbNucAboutDialog : public QDialog
{
  Q_OBJECT

public:
  cmbNucAboutDialog(QWidget* Parent);
  virtual ~cmbNucAboutDialog();

private:
  Ui::AboutDialog* const Ui;
};

#endif
