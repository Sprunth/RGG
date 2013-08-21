#ifndef __cmbNucMaterialColors_h
#define __cmbNucMaterialColors_h

#include <QMap>
#include <QString>
#include <QColor>

static const QMap<QString, QColor>& NUCLEAR_MaterialColors()
{
  static QMap<QString, QColor> materialColors;
  materialColors.insert("gap", QColor::fromRgbF(0.0, 0.0, 0.0, 0.0));
  materialColors.insert("MaterialBlock", QColor::fromRgbF(.7, .1, .7, 1.0));
  materialColors.insert("c1", QColor::fromRgbF(0.3, 0.5, 1.0, 1.0));
  materialColors.insert("m3", QColor::fromRgbF(1.0, 0.1, 0.1, 1.0));
  materialColors.insert("fuel1", QColor::fromRgbF(1.0, 0.1, 0.1, 1.0));
  materialColors.insert("fuel2", QColor::fromRgbF(1.0, 0.5, 0.5, 1.0));
  materialColors.insert("cntr1", QColor::fromRgbF(0.4, 1.0, 0.4, 1.0));
  materialColors.insert("graphite", QColor::fromRgbF(.4, .4, .4, 1.0));
  materialColors.insert("metal", QColor::fromRgbF(.6, .6, .6, 1.0));
  materialColors.insert("coolant", QColor::fromRgbF(0.3, 0.5, 1.0, 1.0));
  materialColors.insert("fuel_uox1", QColor::fromRgbF(0.694, 0.0, 0.149, 1.0));
  materialColors.insert("fuel_uox2", QColor::fromRgbF(0.890, 0.102, 0.110, 1.0));
  materialColors.insert("mox_43", QColor::fromRgbF(0.988, 0.306, 0.165, 1.0));
  materialColors.insert("mox_73", QColor::fromRgbF(0.992, 0.553, 0.235, 1.0));
  materialColors.insert("mox_87", QColor::fromRgbF(0.996, 0.698, 0.298, 1.0));
  materialColors.insert("water", QColor::fromRgbF(0.651, 0.741, 0.859, 0.5));
  materialColors.insert("water_rod", QColor::fromRgbF(0.212, 0.565, 0.753, 1.0));
  materialColors.insert("barod16", QColor::fromRgbF(0.000, 0.427, 0.173, 1.0));
  materialColors.insert("barod18", QColor::fromRgbF(0.192, 0.639, 0.329, 1.0));
  materialColors.insert("barod28", QColor::fromRgbF(0.455, 0.769, 0.463, 1.0));
  materialColors.insert("control_rod", QColor::fromRgbF(0.729, 0.894, 0.702, 1.0));
  materialColors.insert("cladding", QColor::fromRgbF(0.75, 0.75, 0.75, 1.0));

  // default pin and duct color
  materialColors.insert("pin", QColor::fromRgbF(1.0, 0.1, 0.1));
  materialColors.insert("duct", QColor::fromRgbF(1.0, 1.0, 1.0));
  return materialColors;
}

#endif
