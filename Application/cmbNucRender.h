#ifndef __cmbNucRender_h
#define __cmbNucRender_h

#include <QObject>
#include <string>
#include <map>

#include "cmbNucMaterial.h"
#include "cmbNucAssembly.h"
#include "cmbNucCore.h"

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkBoundingBox.h>
#include <vector>

class vtkCompositePolyDataMapper2;
class vtkActor;
class vtkGlyph3DMapper;
class cmbNucRenderHelper;
class vtkRenderer;
class vtkCmbLayeredConeSource;

class cmbNucRender : public QObject
{
  Q_OBJECT
public:

  struct key
  {
    enum {Cylinder, Frustum, Annulus, Jacket, Sectioned} type;
    int sides;
    double radius[8];
    key();
    key( int s, double rTop, double rBottom );
    key( int s );
    key( int s,
         double rTop1, double rTop2, double rTop3, double rTop4,
         double rBottom1, double rBottom2, double rBottom3, double rBottom4);
    bool operator<(key const& other) const;
  };

  struct point
  {
    double xyz[3];
    point(double x = 0, double y = 0, double z = 0)
    {
      xyz[0] = x; xyz[1] = y; xyz[2] = z;
    }
  };

  struct scale
  {
    double xyz[3];
    scale(double x = 1, double y = 1, double z = 1)
    {
      xyz[0] = x; xyz[1] = y; xyz[2] = z;
    }
  };

  struct GeoToPoints
  {
    struct data
    {
      data(point p = point(), point r = point(),
           cmbNucMaterial * m = NULL, scale s = scale())
        : pt(p), rotation(r), material(m), ptScale(s)
      {}

      point pt;
      point rotation;
      scale ptScale;
      cmbNucMaterial * material;
    };
    vtkSmartPointer<vtkPolyData> geo;
    std::vector<data> points;
  };

  cmbNucRender();

  ~cmbNucRender();

  void addToRender(vtkSmartPointer<vtkRenderer> renderer);

  void clearMappers();

  void computeBounds(vtkBoundingBox &);

  void render(cmbNucCore *);
  void render(cmbNucAssembly *);
  void render(DuctCell* ductCell, bool isHex, bool cutaway);
  void render(PinCell* pinCell, bool isHex, bool cutaway);

  void setZScale(double v);

  static vtkSmartPointer<vtkCmbLayeredConeSource> CreateLayerManager(PinCell* pincell, bool isHex, size_t j);
  static vtkSmartPointer<vtkCmbLayeredConeSource> CreateLayerManager(DuctCell* ductCell, bool isHex, size_t j);

  void debug();

protected:
  //vtkSmartPointer<vtkCompositePolyDataMapper2> PolyMapper;
  //vtkSmartPointer<vtkActor> PolyActor;

  vtkSmartPointer<vtkGlyph3DMapper> GlyphMapper;
  vtkSmartPointer<vtkActor> GlyphActor;

  vtkSmartPointer<vtkGlyph3DMapper> TransparentMapper;
  vtkSmartPointer<vtkActor> TransparentActor;

  void sendToGlyphMappers(std::map<key, GeoToPoints> & data);
  vtkBoundingBox BoundingBox;

};

#endif
