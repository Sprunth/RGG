#include "cmbNucRender.h"
#include <vtkCompositePolyDataMapper2.h>
#include <vtkActor.h>
#include <vtkGlyph3DMapper.h>
#include <vtkProperty.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkRenderer.h>
#include <vtkPoints.h>
#include <vtkIntArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointData.h>
#include <vtkBoundingBox.h>
#include <vtkDoubleArray.h>
#include <vtkPlane.h>
#include <vtkNew.h>
#include <vtkMath.h>
#include <vtkClipClosedSurface.h>
#include <vtkPlaneCollection.h>
#include <vtkPolyDataNormals.h>
#include <QDebug>
#include <cmath>

#include "cmbNucDefaults.h"
#include "vtkCmbLayeredConeSource.h"
#include "cmbNucMaterialColors.h"
#include "cmbNucPartDefinition.h"

typedef cmbNucRender::point point;
typedef cmbNucRender::GeoToPoints GeoToPoints;
typedef cmbNucRender::key key;

const int PinCellResolution = 18;

cmbNucRender::key::key()
{
  type = Jacket;
  sides = -1;
  for(int r = 0; r < 8; ++r) radius[r] = -1;
}

cmbNucRender::key::key( int s, double rTop, double rBottom )
{
  type = Frustum;
  sides = s;
  radius[0] = rTop;
  radius[1] = rBottom;
  for(int r = 2; r < 8; ++r) radius[r] = -1;
}

cmbNucRender::key::key( int s )
{
  type = Cylinder;
  sides = s;
  for(int r = 0; r < 8; ++r) radius[r] = -1;
}

cmbNucRender::key::key( int s,
    double rTop1, double rTop2, double rTop3, double rTop4,
    double rBottom1, double rBottom2, double rBottom3, double rBottom4)
{
  type = Annulus;
  sides = s;
  radius[0] = rTop1;
  radius[1] = rBottom1;
  radius[2] = rTop2;
  radius[3] = rBottom2;
  radius[4] = rTop3;
  radius[5] = rBottom3;
  radius[6] = rTop4;
  radius[7] = rBottom4;
}

bool cmbNucRender::key::operator<(key const& other) const
{
  if(type != other.type) return type < other.type;
  switch(type)
  {
    case Annulus:
    case Frustum:
      if(sides == other.sides)
      {
        for(unsigned int i = 0; i < 8; ++i)
        {
          if(radius[i] != other.radius[i])
          {
            return radius[i] < other.radius[i];
          }
        }
      }
    case Cylinder:
    case Sectioned:
      return sides < other.sides;
    case Jacket:
      return false;
  }
  return false;
}

class cmbNucRenderHelper
{
public:

  static void hexHelper(cmbNucCore * input,
                        double & extraXTrans, double & extraYTrans,
                        std::map< std::string, std::vector<point> > & idToPoint)
  {
    Lattice & lat = input->getLattice();
    extraYTrans = 0;
    extraXTrans = 0;
    cmbNucAssembly * assy = input->GetAssembly(0);
    if(assy == NULL) return;
    Duct * dc = assy->getAssyDuct().getDuct(0);
    if(dc == NULL) return;
    double startX = dc->x;
    double startY = dc->y;
    double outerDuctHeight = dc->thickness[0];

    int subType = lat.GetGeometrySubType();
    double LocalCosSinAngles[6][2] =
    { {cmbNucCore::CosSinAngles[0][0],cmbNucCore::CosSinAngles[0][1]},
      {cmbNucCore::CosSinAngles[1][0],cmbNucCore::CosSinAngles[1][1]},
      {cmbNucCore::CosSinAngles[2][0],cmbNucCore::CosSinAngles[2][1]},
      {cmbNucCore::CosSinAngles[3][0],cmbNucCore::CosSinAngles[3][1]},
      {cmbNucCore::CosSinAngles[4][0],cmbNucCore::CosSinAngles[4][1]},
      {cmbNucCore::CosSinAngles[5][0],cmbNucCore::CosSinAngles[5][1]}
    };

    if( (subType & ANGLE_360) && lat.getSize()>=1 )
    {
      double odc = outerDuctHeight*lat.getSize();
      double tmp = odc - outerDuctHeight;
      double t2 = tmp*0.5;
      double ty = std::sqrt(tmp*tmp-t2*t2);
      extraYTrans = -ty;
      extraXTrans = odc;
    }
    else if( (subType & ANGLE_60) && (subType & VERTEX) )
    {
      for(unsigned int i = 0; i < 6; ++i)
      {
        int at = ((5-i) + 3)%6;
        LocalCosSinAngles[i][0] = cmbNucCore::CosSinAngles[at][1];
        LocalCosSinAngles[i][1] = cmbNucCore::CosSinAngles[at][0];
      }
    }
    double hexDiameter, junkDK;
    input->GetDefaults()->getDuctThickness(hexDiameter, junkDK);

    for(size_t i = 0; i < lat.getSize(); i++)
    {
      double layerCorners[6][2], layerRadius;
      if(i>0)
      {
        layerRadius = hexDiameter * i;
        for(int c = 0; c < 6; c++)
        {
          // Needs a little more thinking on math here?
          layerCorners[c][0] = layerRadius * LocalCosSinAngles[c][0];
          layerCorners[c][1] = layerRadius * LocalCosSinAngles[c][1];
        }
      }

      for(size_t j = 0; j < lat.getSize(i); j++)
      {
        if(!lat.GetCell(i,j).isBlank())
        {
          const std::string &type = lat.GetCell(i,j).label;
          cmbNucAssembly* assembly = input->GetAssembly(type);
          if(assembly == NULL) continue;

          // For hex geometry type, figure out the six corners first

          double tX=startX, tY=startY;
          int cornerIdx;
          if(i == 1)
          {
            cornerIdx = j%6;
            tX += layerCorners[cornerIdx][0];
            tY += layerCorners[cornerIdx][1];
          }
          else if( i > 1)
          {
            cornerIdx = j/i;
            int idxOnEdge = j%i;
            if(idxOnEdge == 0) // one of the corners
            {
              tX += layerCorners[cornerIdx][0];
              tY += layerCorners[cornerIdx][1];
            }
            else
            {
              // for each layer, we should have (numLayers-2) middle hexes
              // between the corners
              double deltx, delty, numSegs = i, centerPos[2];
              int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
              deltx = (layerCorners[idxNext][0] - layerCorners[cornerIdx][0]) / numSegs;
              delty = (layerCorners[idxNext][1] - layerCorners[cornerIdx][1]) / numSegs;
              centerPos[0] = layerCorners[cornerIdx][0] + deltx * (idxOnEdge);
              centerPos[1] = layerCorners[cornerIdx][1] + delty * (idxOnEdge);
              tX += centerPos[0];
              tY += centerPos[1];
            }
          }
          idToPoint[type].push_back(point(tX,tY));
        }
      }
    }
  }

  static void hexHelper(cmbNucAssembly * input,
                        double & extraXTrans, double & extraYTrans,
                        std::map< std::string, std::vector<point> > & idToPoint)
  {
    extraXTrans = 0;
    extraYTrans = 0;
    Duct *hexDuct = input->getAssyDuct().getDuct(0);
    Lattice & lat = input->getLattice();
    double pitchX = input->getPinPitchX();
    double pitchY = input->getPinPitchY();

    for(size_t i = 0; i < lat.getSize(); i++)
    {

      for(size_t j = 0; j < lat.getSize(i); j++)
      {
        if(!lat.GetCell(i,j).isBlank())
        {
          const std::string &type = lat.GetCell(i,j).label;
          PinCell* pincell = input->GetPinCell(type);
          if(pincell && (pincell->GetNumberOfParts())>0)
          {
            double pinDistFromCenter = pitchX * (i);
            double tX=hexDuct->x, tY=hexDuct->y;
            int cornerIdx;
            if(i == 1)
            {
              cornerIdx = j%6;
              tX += pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][0];
              tY += pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][1];
            }
            else if( i > 1)
            {
              cornerIdx = j / i;
              int idxOnEdge = j%i;
              if(idxOnEdge == 0) // one of the corners
              {
                tX += pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][0];
                tY += pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][1];
              }
              else
              {
                // for each layer, we should have (numLayers-2) middle hexes
                // between the corners
                double deltx, delty, numSegs = i, centerPos[2];
                int idxNext = cornerIdx==5 ? 0 : cornerIdx+1;
                deltx = pinDistFromCenter*(cmbNucAssembly::CosSinAngles[idxNext][0] - cmbNucAssembly::CosSinAngles[cornerIdx][0]) / numSegs;
                delty = pinDistFromCenter*(cmbNucAssembly::CosSinAngles[idxNext][1] - cmbNucAssembly::CosSinAngles[cornerIdx][1]) / numSegs;
                centerPos[0] = pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][0] + deltx * (idxOnEdge);
                centerPos[1] = pinDistFromCenter*cmbNucAssembly::CosSinAngles[cornerIdx][1] + delty * (idxOnEdge);
                tX += centerPos[0];
                tY += centerPos[1];
              }
            }
            idToPoint[type].push_back(point(tX, -tY));
          }
        }
      }
    }
  }

  template<class T>
  static void helperRect(T * input,
                         double & extraXTrans, double & extraYTrans,
                         std::map< std::string, std::vector<point> > & idToPoint)
  {
    double currentLaticePoint[] = {0,0};

    Lattice & lat = input->getLattice();

    for(size_t i = 0; i < lat.getSize(); i++)
    {
      for(size_t j = 0; j < lat.getSize(i); j++)
      {
        input->calculateRectPt(i, j, currentLaticePoint);
        if(!lat.GetCell(i,j).isBlank())
        {
          idToPoint[lat.GetCell(i,j).label].push_back(point( currentLaticePoint[0],
                                                             currentLaticePoint[1]));
        }
      }
    }
    input->calculateRectTranslation(currentLaticePoint, extraXTrans, extraYTrans);
  }

  template<class T>
  static void calculatePoints( T * input, double & extraXTrans, double & extraYTrans,
                               std::map< std::string, std::vector<point> > & idToPoint)
  {
    extraXTrans = 0;
    extraYTrans = 0;
    if(input->IsHexType())
    {
      hexHelper(input, extraXTrans, extraYTrans, idToPoint);
    }
    else
    {
      helperRect(input, extraXTrans, extraYTrans, idToPoint);
    }
  }

  static void createGeo(DuctCell* ductcell, bool isHex, std::map<key,GeoToPoints> & geometry)
  {
    if(ductcell == NULL) return;
    size_t numParts = ductcell->numberOfDucts();
    for(unsigned int j = 0; j < numParts; ++j)
    {
      Duct *duct = ductcell->getDuct(j);
      vtkSmartPointer<vtkCmbLayeredConeSource> manager = cmbNucRender::CreateLayerManager(ductcell, isHex, j);
      //inner cylinder
      {
        double thickness[] = {manager->GetTopRadius(0,0), manager->GetTopRadius(0,1)};
        key k (manager->GetResolution(0));
        point loc;
        manager->GetBaseCenter(loc.xyz);
        double h = manager->GetHeight();
        cmbNucRender::scale scale(thickness[0],thickness[1],h);
        if(geometry.find(k) == geometry.end())
        {
          geometry[k].geo = manager->CreateUnitLayer(0);
        }
        GeoToPoints & geo = geometry[k];
        geo.points.push_back(GeoToPoints::data(loc, point(), duct->getMaterial(0), scale));
      }
      for( int i = manager->GetNumberOfLayers()-1; i > 0; --i)
      {
        cmbNucMaterial * mat = duct->getMaterial(i);
        key k(manager->GetResolution(i),
              manager->GetTopRadius(i, 0), manager->GetTopRadius(i, 1),
              manager->GetTopRadius(i-1, 0), manager->GetTopRadius(i-1, 1),
              manager->GetBaseRadius(i,0), manager->GetBaseRadius(i,1),
              manager->GetBaseRadius(i-1,0), manager->GetBaseRadius(i-1,1));
        if(geometry.find(k) == geometry.end())
        {
          geometry[k].geo = manager->CreateUnitLayer(i);
        }
        GeoToPoints & geo = geometry[k];
        point loc;
        manager->GetBaseCenter(loc.xyz);
        double h = manager->GetHeight();
        cmbNucRender::scale scale(1,1,h);
        geo.points.push_back(GeoToPoints::data(loc, point(), mat, scale));
      }
    }
  }

  static void createGeo(PinCell* pincell, bool isHex,
                        std::map<key,GeoToPoints> & geometry,
                        double pitchX = -1, double pitchY = -1 )
  {
    if(pincell == NULL) return;
    size_t numParts = pincell->GetNumberOfParts();
    point rotate(0,0,(isHex)?30:0);

    for(size_t j = 0; j < numParts; ++j)
    {
      PinSubPart* part = pincell->GetPart(j);
      vtkSmartPointer<vtkCmbLayeredConeSource> manager = cmbNucRender::CreateLayerManager(pincell, isHex, j, pitchX, pitchY);
      if(manager == NULL) continue;
      //inner cylinder/frustum
      {
        double thickness[] = {manager->GetTopRadius(0), manager->GetBaseRadius(0)};
        key k (manager->GetResolution(0));
        point loc;
        manager->GetBaseCenter(loc.xyz);
        double h = manager->GetHeight();
        cmbNucRender::scale scale(thickness[0],thickness[0],h);
        if(thickness[0] != thickness[1])
        {
          k = key(manager->GetResolution(0), thickness[0], thickness[1]);
          scale = cmbNucRender::scale(1,1,h);
        }
        if(geometry.find(k) == geometry.end())
        {
          geometry[k].geo = manager->CreateUnitLayer(0);
        }
        GeoToPoints & geo = geometry[k];
        geo.points.push_back(GeoToPoints::data(loc, rotate, part->GetMaterial(0), scale));
      }

      bool addCellMat = pincell->cellMaterialSet();
      for( int i = manager->GetNumberOfLayers()-1; i > 0; --i)
      {
        cmbNucMaterial * mat = part->GetMaterial(i);

        if(addCellMat)
        {
          addCellMat = false;
          mat = pincell->getCellMaterial();
        }
        key k(manager->GetResolution(i),
              manager->GetTopRadius(i, 0), manager->GetTopRadius(i, 1),
              manager->GetTopRadius(i-1, 0), manager->GetTopRadius(i-1, 1),
              manager->GetBaseRadius(i,0), manager->GetBaseRadius(i,1),
              manager->GetBaseRadius(i-1,0), manager->GetBaseRadius(i-1,1));
        if(geometry.find(k) == geometry.end())
        {
          geometry[k].geo = manager->CreateUnitLayer(i);
        }
        GeoToPoints & geo = geometry[k];
        point loc;
        manager->GetBaseCenter(loc.xyz);
        double h = manager->GetHeight();
        cmbNucRender::scale scale(1,1,h);
        geo.points.push_back(GeoToPoints::data(loc, rotate, mat, scale));
      }
    }
  }

  static void createGeo(cmbNucAssembly * input, bool rotate_30, std::map<key,GeoToPoints> & geometry)
  {
    std::map< std::string, std::vector<point> > idToPoint;
    double extraXTrans = 0;
    double extraYTrans = 0;
    double pitchX = input->getPinPitchX();
    double pitchY = input->getPinPitchY();
    calculatePoints(input, extraXTrans, extraYTrans, idToPoint);
    point xformR;
    point xformS;
    bool hasSectioning = false;
    std::vector<point> sectioningPlanes;
    static int sectionedCount = 0;
    if(rotate_30)
    {
      xformR.xyz[2] += 30;
      xformS.xyz[2] -= 30;
    }
    for(unsigned int i = 0; i < input->getNumberOfTransforms(); ++i)
    {
      cmbNucAssembly::Transform* tmpxf = input->getTransform(i);
      if( tmpxf == NULL ) continue;
      if(tmpxf->getLabel() == "Rotate")
      {
        xformR.xyz[tmpxf->getAxis()] += tmpxf->getValue();
        xformS.xyz[tmpxf->getAxis()] -= tmpxf->getValue();
      }
      else
      {
        hasSectioning = true;
        point plane;
        plane.xyz[tmpxf->getAxis()] = (tmpxf->reverse())?-1:1;
        transformNormal(plane.xyz, xformS.xyz);
        sectioningPlanes.push_back(plane);
      }

    }
    //Duct
    {
      std::map<key, GeoToPoints> tmpGeo;
      //prepopulate the keys
      for(std::map<key, GeoToPoints>::const_iterator i = geometry.begin(); i != geometry.end(); ++i)
      {
        tmpGeo[i->first].geo = i->second.geo;
      }
      createGeo(&(input->getAssyDuct()), input->IsHexType(), tmpGeo);
      std::vector<point> points(1);

      if(hasSectioning)
      {
        for(std::map<key, GeoToPoints>::iterator i = tmpGeo.begin(); i != tmpGeo.end(); ++i)
        {
          //NOTE: assuming _|_ to z axis.
          if(i->second.points.empty()) continue;
          key k = i->first;
          k.type = cmbNucRender::key::Sectioned;
          k.sides = sectionedCount++;
          GeoToPoints data = i->second;
          for(unsigned int j = 0; j < sectioningPlanes.size(); ++j)
          {
            point plane = sectioningPlanes[j];
            double tmXf[3] = {-data.points[0].rotation.xyz[0],
                              -data.points[0].rotation.xyz[1],
                              -data.points[0].rotation.xyz[2]};
            cmbNucRenderHelper::transformNormal(plane.xyz, tmXf);
            clip(data.geo, data.geo, plane.xyz, 0);
          }
          addGeometry( k, data, points, extraXTrans, extraYTrans, geometry, xformR );
        }
      }
      else
      {
        mergeGeometry(tmpGeo, points, 0, 0, geometry, xformR);
      }
    }
    //pins
    for(std::map< std::string, std::vector<point> >::const_iterator iter = idToPoint.begin();
        iter != idToPoint.end(); ++iter)
    {
      std::map<key, GeoToPoints> tmpGeo;
      //prepopulate the keys
      for(std::map<key, GeoToPoints>::const_iterator i = geometry.begin(); i != geometry.end(); ++i)
      {
        tmpGeo[i->first].geo = i->second.geo;
      }

      createGeo(input->GetPinCell(iter->first), input->IsHexType(), tmpGeo, pitchX, pitchY);

      std::vector<point> const& points = iter->second;
      if(hasSectioning)
      {
        for(std::map<key, GeoToPoints>::iterator i = tmpGeo.begin(); i != tmpGeo.end(); ++i)
        {
          double radius = -1;
          for(unsigned int k = 0; k < 8; ++k)
          {
            radius = std::max(radius, i->first.radius[k]);
          }
          if(radius == -1) radius = 1;
          GeoToPoints const& data = i->second;
          GeoToPoints keep;
          keep.geo = data.geo;
          //NOTE: assuming _|_ to z axis.
          for(unsigned int pt = 0; pt < data.points.size(); ++pt)
          {
            GeoToPoints::data dp = data.points[pt];
            double currentR = radius * std::max(dp.ptScale.xyz[0],dp.ptScale.xyz[1]);
            for(unsigned int z = 0; z < points.size(); ++z)
            {
              GeoToPoints::data cp = computeData( dp, points[z], extraXTrans, extraYTrans, 0);
              enum {KEEP_PT, CROPPED_PT, IGNORE_PT} mode = KEEP_PT;
              key k = i->first;
              k.type = cmbNucRender::key::Sectioned;
              k.sides = sectionedCount++;
              GeoToPoints ndp;
              ndp.geo = data.geo;
              for(unsigned int j = 0; j < sectioningPlanes.size(); ++j)
              {
                vtkNew<vtkPlane> testPlane;
                point plane = sectioningPlanes[j];
                testPlane->SetOrigin(0, 0, 0);
                testPlane->SetNormal(plane.xyz[0], plane.xyz[1], plane.xyz[2]);
                double tpdist = testPlane->EvaluateFunction(cp.pt.xyz);
                if( tpdist < -currentR )
                {
                  mode = IGNORE_PT;
                  continue;
                }
                else if( std::abs(tpdist) <= currentR && mode != IGNORE_PT )
                {
                  mode = CROPPED_PT;
                  double tmXf[3] = {-dp.rotation.xyz[0],
                                    -dp.rotation.xyz[1],
                                    -dp.rotation.xyz[2]};
                  transformNormal(plane.xyz, tmXf);
                  clip(ndp.geo, ndp.geo, plane.xyz, 1);
                }
              }
              switch(mode)
              {
                case KEEP_PT:
                  keep.points.push_back(cp);
                  break;
                case CROPPED_PT:
                  ndp.points.push_back(cp);
                  addGeometry( k, ndp, std::vector<point>(1), 0, 0, geometry, xformR);
                  break;
                default: break;//do nothing
              }
            }
          }
          addGeometry( i->first, keep, std::vector<point>(1), 0, 0, geometry, xformR);
        }
      }
      else
      {
        mergeGeometry(tmpGeo, points, extraXTrans, extraYTrans, geometry, xformR);
      }
      //start of delete
    }
  }

  static void createGeo(cmbNucCore * input, std::map<key, GeoToPoints> & geometry)
  {
    std::map< std::string, std::vector<point> > idToPoint;
    double extraXTrans;
    double extraYTrans;
    calculatePoints(input, extraXTrans, extraYTrans, idToPoint);
    bool rotate_30 = input->IsHexType() && !(input->getLattice().GetGeometrySubType()&VERTEX);
    for( std::map< std::string, std::vector<point> >::const_iterator iter = idToPoint.begin();
        iter != idToPoint.end(); ++iter)
    {
      cmbNucAssembly* assembly = input->GetAssembly(iter->first);
      if(!assembly)
      {
        continue;
      }
      std::map<key, GeoToPoints> tmpGeo;
      //prepopulate the keys
      for(std::map<key, GeoToPoints>::const_iterator i = geometry.begin(); i != geometry.end(); ++i)
      {
        tmpGeo[i->first].geo = i->second.geo;
      }

      createGeo(assembly, rotate_30, tmpGeo);
      std::vector<point> const& points = iter->second;
      mergeGeometry(tmpGeo, points, extraXTrans, extraYTrans, geometry);
    }
  }

  static void transform(double * xyz, point xformR)
  {
    if(xformR.xyz[0] != 0 || xformR.xyz[1] != 0 || xformR.xyz[2] != 0)
    {
      vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
      xform->RotateX(xformR.xyz[0]);
      xform->RotateY(xformR.xyz[1]);
      xform->RotateZ(xformR.xyz[2]);
      double tp[3];
      xform->TransformVector(xyz,tp);
      xyz[0] = tp[0];
      xyz[1] = tp[1];
      xyz[2] = tp[2];
    }
  }

  static void transformNormal(double * xyz, double * xformR)
  {
    if(xformR[0] != 0 || xformR[1] != 0 || xformR[2] != 0)
    {
      vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
      xform->RotateX(xformR[0]);
      xform->RotateY(xformR[1]);
      xform->RotateZ(xformR[2]);
      double tp[3];
      xform->TransformNormal(xyz,tp);
      xyz[0] = tp[0];
      xyz[1] = tp[1];
      xyz[2] = tp[2];
      normalize(xyz);
    }
  }

  static void normalize(double *xyz)
  {
    double sum = std::sqrt(xyz[0]*xyz[0]+ xyz[1]*xyz[1] + xyz[2]*xyz[2]);
    if(sum == 0) return;
    xyz[0] /= sum;
    xyz[1] /= sum;
    xyz[2] /= sum;
  }

  static GeoToPoints::data computeData( GeoToPoints::data const& pt, point const& apt,
                                        double extraXTrans, double extraYTrans,
                                        point xformR = point())
  {
    point tranpt = pt.pt;
    tranpt.xyz[0] += apt.xyz[0] + extraXTrans;
    tranpt.xyz[1] += apt.xyz[1] + extraYTrans;
    transform(tranpt.xyz, xformR);
    point rotation = pt.rotation;
    rotation.xyz[0] += xformR.xyz[0];
    rotation.xyz[1] += xformR.xyz[1];
    rotation.xyz[2] += xformR.xyz[2];
    return GeoToPoints::data(tranpt,rotation, pt.material, pt.ptScale);
  }

  static void addGeometry( key keyIn, GeoToPoints const& tmpGeoI,
                           std::vector<point> const& points,
                           double extraXTrans, double extraYTrans,
                           std::map<key, GeoToPoints> & geometry,
                           point xformR = point())
  {
    GeoToPoints &newGeo = geometry[keyIn];
    if(newGeo.geo == NULL)
    {
      newGeo.geo = tmpGeoI.geo;
    }

    unsigned int at = newGeo.points.size();
    newGeo.points.resize(newGeo.points.size() + points.size()*tmpGeoI.points.size());
    for(unsigned int j = 0; j<points.size(); ++j)
    {
      point const& apt = points[j];
      for(unsigned int k = 0; k < tmpGeoI.points.size(); ++k)
      {
        newGeo.points[at] = computeData( tmpGeoI.points[k], apt,
                                         extraXTrans, extraYTrans,xformR);
        ++at;
      }
    }
  }

  static void mergeGeometry(std::map<key, GeoToPoints> const& in,
                            std::vector<point> const& points,
                            double extraXTrans, double extraYTrans,
                            std::map<key, GeoToPoints> & geometry,
                            point xformR = point())
  {
    for(std::map<key, GeoToPoints>::const_iterator i = in.begin(); i != in.end(); ++i)
    {
      addGeometry( i->first, i->second, points, extraXTrans, extraYTrans, geometry, xformR);
    }
  }

  static void clip(vtkSmartPointer<vtkPolyData> in,
                   vtkSmartPointer<vtkPolyData> &out,
                   double *normal, int offset = 0)
  {
    vtkSmartPointer<vtkPolyData> tmpIn = in;
    out = vtkSmartPointer<vtkPolyData>::New();
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(-normal[0]*0.005*offset, -normal[1]*0.005*offset, -normal[2]*0.005*offset);
    plane->SetNormal(normal[0], normal[1], normal[2]);

    vtkNew<vtkClipClosedSurface> clipper;
    vtkNew<vtkPlaneCollection> clipPlanes;
    vtkNew<vtkPolyDataNormals> normals;
    clipPlanes->AddItem(plane.GetPointer());
    clipper->SetClippingPlanes(clipPlanes.GetPointer());
    clipper->SetActivePlaneId(0);
    clipper->SetClipColor(1.0,1.0,1.0);
    clipper->SetActivePlaneColor(1.0,1.0,0.8);
    clipper->GenerateOutlineOff();
    clipper->SetInputData(tmpIn);
    clipper->GenerateFacesOn();
    normals->SetInputConnection(clipper->GetOutputPort());
    normals->Update();
    out->DeepCopy(normals->GetOutput());
  }

  static void addOuterJacket(cmbNucCore * core, std::map<key, GeoToPoints> & geometry)
  {
    if(!core->getHasCylinder())
    {
      return;
    }

    double r = core->getCylinderRadius();

    int s = core->getCylinderOuterSpacing();

    vtkSmartPointer<vtkCmbLayeredConeSource> cylinder = vtkSmartPointer<vtkCmbLayeredConeSource>::New();

    cmbNucAssembly * assy = core->GetAssembly(0);
    double outerDuctWidth = assy->getAssyDuct().getDuct(0)->thickness[1];
    double outerDuctHeight = assy->getAssyDuct().getDuct(0)->thickness[0];
    double extraXTrans = 0, extraYTrans = 0;

    if(core->IsHexType())
    {
      double odc = outerDuctHeight*core->getLattice().getSize();
      double tmp = odc - outerDuctHeight;
      double t2 = tmp*0.5;
      double ty = std::sqrt(tmp*tmp-t2*t2);
      extraYTrans = -ty;
      extraXTrans = odc;
    }
    else
    {
      double tx = outerDuctWidth*(core->getLattice().getSize()-1)*0.5;
      double ty = outerDuctHeight*(core->getLattice().getSize(0)-1)*0.5;
      extraXTrans = ty,
      extraYTrans = tx-outerDuctWidth*(core->getLattice().getSize()-1);
    }

    cylinder->SetHeight(1);
    cylinder->SetNumberOfLayers(1);
    cylinder->SetTopRadius(0, r);
    cylinder->SetBaseRadius(0, r);
    cylinder->SetResolution(0, s);
    double odc = outerDuctHeight*(core->getLattice().getSize()-1);
    double tmp = (outerDuctHeight*0.5) / (cos(30.0 * vtkMath::Pi() / 180.0));
    double pt1[] = {odc * cmbNucCore::CosSinAngles[5][0], odc * cmbNucCore::CosSinAngles[5][1]};
    double pt2[2];

    static const double AssyCosSinAngles[6][2] =
    { { cos( (vtkMath::Pi() / 3.0) * (0 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (0 + 3.5) ) },
      { cos( (vtkMath::Pi() / 3.0) * (1 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (1 + 3.5) ) },
      { cos( (vtkMath::Pi() / 3.0) * (2 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (2 + 3.5) ) },
      { cos( (vtkMath::Pi() / 3.0) * (3 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (3 + 3.5) ) },
      { cos( (vtkMath::Pi() / 3.0) * (4 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (4 + 3.5) ) },
      { cos( (vtkMath::Pi() / 3.0) * (5 + 3.5) ),
        sin( (vtkMath::Pi() / 3.0) * (5 + 3.5) ) } };
    if(core->IsHexType())
    {
      for(int i = 0; i < 6; ++i)
      {
        int at = 5-i-1;
        if(at<0) at = 5;
        pt2[0] = odc * cmbNucCore::CosSinAngles[at][0];
        pt2[1] = odc * cmbNucCore::CosSinAngles[at][1];
        int sp1 = (i + 0)%6;
        int sp2 = (i + 1)%6;
        if(core->getLattice().getSize() == 1)
        {
          cylinder->addInnerPoint(pt1[0]+tmp * AssyCosSinAngles[sp1][0],
                                  pt1[1]+tmp * AssyCosSinAngles[sp1][1]);
        }
        else for(size_t j = 0; j < core->getLattice().getSize(); ++j)
        {
          double tmps = j/(core->getLattice().getSize()-1.0);
          double pt[] = {pt1[0]*(1.0-tmps)+pt2[0]*(tmps), pt1[1]*(1.0-tmps)+pt2[1]*(tmps)};

          {
            cylinder->addInnerPoint(pt[0]+tmp * AssyCosSinAngles[sp1][0],
                                    pt[1]+tmp * AssyCosSinAngles[sp1][1]);
          }
          if(j != core->getLattice().getSize()-1 )
          {
            cylinder->addInnerPoint(pt[0]+tmp * AssyCosSinAngles[sp2][0],
                                    pt[1]+tmp * AssyCosSinAngles[sp2][1]);
          }
        }

        pt1[0] = pt2[0];
        pt1[1] = pt2[1];
      }
    }
    else
    {
      double height = outerDuctWidth*(core->getLattice().getSize())*0.5;
      double width = outerDuctHeight*(core->getLattice().getSize(0))*0.5;

      cylinder->addInnerPoint( width,-height);
      cylinder->addInnerPoint( width, height);
      cylinder->addInnerPoint(-width, height);
      cylinder->addInnerPoint(-width,-height);
      
    }
    double bc[] = {extraXTrans, extraYTrans, 0};
    cylinder->SetBaseCenter(bc);

    point loc;
    cylinder->GetBaseCenter(loc.xyz);

    key k;
    GeoToPoints & geo = geometry[k];
    geo.geo = cylinder->CreateUnitLayer(0);
    double h;
    core->GetDefaults()->getHeight(h);
    cmbNucMaterialColors* matColorMap = cmbNucMaterialColors::instance();
    geo.points.push_back(GeoToPoints::data( loc, point(),matColorMap->getUnknownMaterial(),
                                            cmbNucRender::scale(1,1,h)));

  }
};

cmbNucRender::cmbNucRender()
 :GlyphMapper(vtkSmartPointer<vtkGlyph3DMapper>::New()),
  GlyphActor(vtkSmartPointer<vtkActor>::New()),
  TransparentMapper(vtkSmartPointer<vtkGlyph3DMapper>::New()),
  TransparentActor(vtkSmartPointer<vtkActor>::New())
{

  this->GlyphActor->SetMapper(this->GlyphMapper.GetPointer());
  this->GlyphActor->GetProperty()->SetShading(1);
  this->GlyphActor->GetProperty()->SetInterpolationToPhong();

  this->TransparentActor->SetMapper(this->TransparentMapper.GetPointer());
  this->TransparentActor->GetProperty()->SetShading(1);
  this->TransparentActor->GetProperty()->SetInterpolationToPhong();
  this->TransparentActor->GetProperty()->SetOpacity(0.99);
}

cmbNucRender::~cmbNucRender()
{
}

void cmbNucRender::addToRender(vtkSmartPointer<vtkRenderer> renderer)
{
  renderer->AddActor(this->GlyphActor);
  renderer->AddActor(this->TransparentActor);
}

void cmbNucRender::clearMappers()
{
  GlyphMapper->RemoveAllInputConnections(1);
  GlyphMapper->SetInputData(NULL);
  TransparentMapper->RemoveAllInputConnections(1);
  TransparentMapper->SetInputData(NULL);
}

void cmbNucRender::render(cmbNucCore * core)
{
  std::map<key, GeoToPoints> geometry;
  cmbNucRenderHelper::createGeo(core, geometry);
  cmbNucRenderHelper::addOuterJacket(core, geometry);
  BoundingBox = core->computeBounds();
  sendToGlyphMappers(geometry);
}

void cmbNucRender::render(cmbNucAssembly * assy, bool rotate_30)
{
  std::map<key, GeoToPoints> geometry;
  cmbNucRenderHelper::createGeo(assy, rotate_30, geometry);
  BoundingBox = assy->computeBounds();
  sendToGlyphMappers(geometry);
}

void cmbNucRender::render(DuctCell* ductCell, bool isHex, bool cutaway)
{
  std::map<key, GeoToPoints> geometry;
  cmbNucRenderHelper::createGeo(ductCell, isHex, geometry);
  if(cutaway)
  {
    double normal[] = {0, 1, 0};
    for(std::map<key, GeoToPoints>::iterator iter = geometry.begin(); iter != geometry.end(); ++iter)
    {
      cmbNucRenderHelper::clip(iter->second.geo,iter->second.geo,normal);
    }
  }
  BoundingBox = ductCell->computeBounds(isHex);
  sendToGlyphMappers(geometry);
}

void cmbNucRender::render(PinCell* pinCell, bool isHex, bool cutaway)
{
  std::map<key, GeoToPoints> geometry;
  cmbNucRenderHelper::createGeo(pinCell, isHex, geometry);
  if(cutaway)
  {
    for(std::map<key, GeoToPoints>::iterator iter = geometry.begin(); iter != geometry.end(); ++iter)
    {
      double normal[] = {0, 1, 0};
      double xform[] = { -iter->second.points[0].rotation.xyz[0],
                         -iter->second.points[0].rotation.xyz[1],
                         -iter->second.points[0].rotation.xyz[2] };

      cmbNucRenderHelper::transformNormal(normal, xform);
      cmbNucRenderHelper::clip(iter->second.geo, iter->second.geo, normal);
    }
  }
  BoundingBox = pinCell->computeBounds(isHex);
  sendToGlyphMappers(geometry);
}

void cmbNucRender::computeBounds(vtkBoundingBox & in)
{
  in = BoundingBox;
}

void cmbNucRender::setZScale(double v)
{
  this->GlyphActor->SetScale(1, 1, v);
  this->TransparentActor->SetScale(1, 1, v);
}

void cmbNucRender::sendToGlyphMappers(std::map<key, GeoToPoints> & geometry)
{
  clearMappers();
  vtkSmartPointer<vtkPoints> polyPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> polyPointsTrans = vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkIntArray> polyScalers = vtkSmartPointer<vtkIntArray>::New();
  vtkSmartPointer<vtkIntArray> polyScalersTrans = vtkSmartPointer<vtkIntArray>::New();

  vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  vtkSmartPointer<vtkUnsignedCharArray> colorsTrans = vtkSmartPointer<vtkUnsignedCharArray>::New();

  vtkSmartPointer<vtkDoubleArray> rotationAngles = vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkDoubleArray> rotationAnglesTrans = vtkSmartPointer<vtkDoubleArray>::New();

  vtkSmartPointer<vtkDoubleArray> scaleGlyph = vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkDoubleArray> scaleTrans = vtkSmartPointer<vtkDoubleArray>::New();

  colors->SetName("colors");
  colors->SetNumberOfComponents(3);
  colorsTrans->SetName("colors");
  colorsTrans->SetNumberOfComponents(4);


  polyScalers->SetNumberOfComponents(1);
  polyScalers->SetName("Ids");
  polyScalersTrans->SetNumberOfComponents(1);
  polyScalersTrans->SetName("Ids");

  rotationAngles->SetNumberOfComponents(3);
  rotationAngles->SetName("Rotation");
  rotationAnglesTrans->SetNumberOfComponents(3);
  rotationAnglesTrans->SetName("Rotation");

  scaleGlyph->SetNumberOfComponents(3);
  scaleGlyph->SetName("Scale");
  scaleTrans->SetNumberOfComponents(3);
  scaleTrans->SetName("Scale");

  int index = 0;
  int indexTrans = 0;
  for(std::map<key, GeoToPoints>::const_iterator iter = geometry.begin(); iter != geometry.end(); ++iter)
  {
    bool added_to_trans = false;
    bool added_to_solid = false;

    GeoToPoints const& geo = iter->second;
    for(unsigned int j = 0; j < geo.points.size(); ++j)
    {
      geo.points[j].material->setDisplayed();
      if(!geo.points[j].material->isVisible()) continue;
      QColor bColor = geo.points[j].material->getColor();
      unsigned char color[] = { static_cast<unsigned char>(bColor.redF()*255),
                                static_cast<unsigned char>(bColor.greenF()*255),
                                static_cast<unsigned char>(bColor.blueF()*255),
                                static_cast<unsigned char>(bColor.alphaF()*255) };
      vtkSmartPointer<vtkPoints> toutpts = polyPoints;
      vtkSmartPointer<vtkIntArray> toutsc = polyScalers;
      vtkSmartPointer<vtkUnsignedCharArray> toutc = colors;
      vtkSmartPointer<vtkDoubleArray> toutr = rotationAngles;
      vtkSmartPointer<vtkDoubleArray> tscale = scaleGlyph;
      int at = index-1;

      if(bColor.alphaF() == 0) continue;


      if(bColor.alphaF() != 1)
      {
        if(!added_to_trans)
        {
          added_to_trans = true;
          TransparentMapper->SetSourceData(indexTrans++, geo.geo);
        }
        at = indexTrans - 1;
        toutpts = polyPointsTrans;
        toutsc = polyScalersTrans;
        toutc = colorsTrans;
        toutr = rotationAnglesTrans;
        tscale = scaleTrans;
      }
      else if(!added_to_solid)
      {
        added_to_solid = true;
        at = index;
        GlyphMapper->SetSourceData(index++, geo.geo);
      }
      toutpts->InsertNextPoint(geo.points[j].pt.xyz);
      toutr->InsertNextTuple(geo.points[j].rotation.xyz);
      toutsc->InsertNextValue(at);
      toutc->InsertNextTupleValue(color);
      tscale->InsertNextTuple(geo.points[j].ptScale.xyz);
    }
  }

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(polyPoints);
  polydata->GetPointData()->AddArray(polyScalers);
  polydata->GetPointData()->AddArray(rotationAngles);
  polydata->GetPointData()->AddArray(scaleGlyph);
  polydata->GetPointData()->SetScalars(colors);
  GlyphMapper->ScalingOn();
  GlyphMapper->ClampingOff();
  GlyphMapper->SetScaleArray("Scale");
  GlyphMapper->SetScaleModeToScaleByVectorComponents();
  GlyphMapper->SetInputData(polydata);
  GlyphMapper->SourceIndexingOn();
  GlyphMapper->SetSourceIndexArray("Ids");
  GlyphMapper->SetRange(0,index);
  GlyphMapper->OrientOn();
  GlyphMapper->SetOrientationModeToRotation();
  GlyphMapper->SetOrientationArray("Rotation");

  vtkSmartPointer<vtkPolyData> polydataTrans = vtkSmartPointer<vtkPolyData>::New();
  polydataTrans->SetPoints(polyPointsTrans);
  polydataTrans->GetPointData()->AddArray(polyScalersTrans);
  polydataTrans->GetPointData()->AddArray(rotationAnglesTrans);
  polydataTrans->GetPointData()->SetScalars(colorsTrans);
  polydataTrans->GetPointData()->AddArray(scaleTrans);
  TransparentMapper->ScalingOn();
  TransparentMapper->ClampingOff();
  TransparentMapper->SetScaleArray("Scale");
  TransparentMapper->SetScaleModeToScaleByVectorComponents();
  TransparentMapper->SetInputData(polydataTrans);
  TransparentMapper->SourceIndexingOn();
  TransparentMapper->SetSourceIndexArray("Ids");
  TransparentMapper->SetRange(0,indexTrans);
  TransparentMapper->OrientOn();
  TransparentMapper->SetOrientationModeToRotation();
  TransparentMapper->SetOrientationArray("Rotation");

  BoundingBox.AddBounds(polydataTrans->GetBounds());
  BoundingBox.AddBounds(polydata->GetBounds());
}

vtkSmartPointer<vtkCmbLayeredConeSource> cmbNucRender::CreateLayerManager(PinCell* pincell,
                                                                          bool isHex,
                                                                          size_t j,
                                                                          double pitchX,
                                                                          double pitchY)
{
  PinSubPart* part = pincell->GetPart(j);
  
  vtkSmartPointer<vtkCmbLayeredConeSource> coneSource = vtkSmartPointer<vtkCmbLayeredConeSource>::New();
  coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers() + (pincell->cellMaterialSet()?1:0));
  coneSource->SetBaseCenter(0, 0, part->z1);
  coneSource->SetHeight(part->z2 - part->z1);
  double largestRadius = 0;

  for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
  {
    coneSource->SetBaseRadius(k, part->getRadius(k,Frustum::BOTTOM));
    coneSource->SetTopRadius(k, part->getRadius(k,Frustum::TOP));
    coneSource->SetResolution(k, PinCellResolution);
    if(largestRadius < part->getRadius(k,Frustum::BOTTOM))
    {
      largestRadius = part->getRadius(k,Frustum::BOTTOM);
    }
    if(largestRadius < part->getRadius(k,Frustum::TOP))
    {
      largestRadius = part->getRadius(k,Frustum::TOP);
    }
  }
  if(pincell->cellMaterialSet())
  {
    largestRadius *= 2.50;
    if(pitchX == -1 || pitchY == -1)
    {
      pitchX = pitchY = largestRadius;
    }
    double r[] = {pitchX*0.5, pitchY*0.5};
    int res = 4;
    if(isHex)
    {
      res = 6;
      r[0] = r[1] = r[0]/0.86602540378443864676372317075294;
    }
    coneSource->SetBaseRadius(pincell->GetNumberOfLayers(), r[0], r[1]);
    coneSource->SetTopRadius(pincell->GetNumberOfLayers(), r[0], r[1]);
    coneSource->SetResolution(pincell->GetNumberOfLayers(), res);
  }
  double direction[] = { 0, 0, 1 };
  coneSource->SetDirection(direction);
  return coneSource;
}

vtkSmartPointer<vtkCmbLayeredConeSource> cmbNucRender::CreateLayerManager(DuctCell* ductCell, bool isHex, size_t i)
{
  Duct *duct = ductCell->getDuct(i);
  double z = duct->getZ1();
  double height = duct->getZ2() - duct->getZ1();
  double deltaZ = height * 0.0005;
  if(i == 0) // first duct
  {
    z = duct->getZ1() + deltaZ;
    // if more than one duct, first duct height need to be reduced by deltaZ
    height = ductCell->numberOfDucts() > 1 ? height - deltaZ : height - 2*deltaZ;
  }
  else if (i == ductCell->numberOfDucts() - 1) // last duct
  {
    height -= 2*deltaZ;
  }
  else
  {
    z = duct->getZ1() + deltaZ;
  }

  int numLayers = duct->NumberOfLayers();
  vtkSmartPointer<vtkCmbLayeredConeSource> coneSource =
     vtkSmartPointer<vtkCmbLayeredConeSource>::New();
  coneSource->SetNumberOfLayers(numLayers);
  coneSource->SetBaseCenter(duct->x, duct->y, z);
  double direction[] = { 0, 0, 1 };
  coneSource->SetDirection(direction);
  coneSource->SetHeight(height);

  int res = 4;
  double mult = 0.5;

  if(isHex)
  {
    res = 6;
    mult = 0.5/0.86602540378443864676372317075294;
  }

  for(int k = 0; k < numLayers; k++)
  {
    double tx = duct->GetLayerThick(k,0) - duct->GetLayerThick(k,0)*0.0005;
    double ty = duct->GetLayerThick(k,1) - duct->GetLayerThick(k,0)*0.0005;
    coneSource->SetBaseRadius(k, tx * mult, ty*mult);
    coneSource->SetTopRadius(k, tx * mult, ty*mult);
    coneSource->SetResolution(k, res);
  }
  return coneSource;
}

void cmbNucRender::debug()
{
  double * bounds = GlyphActor->GetBounds();
  qDebug() << "Solid glyphs bounds:" << bounds[0] << bounds[1] << bounds[2] << bounds[3] << bounds[4] << bounds[5];
  bounds = TransparentActor->GetBounds();
  qDebug() << "Transparent glyphs bounds:" << bounds[0] << bounds[1] << bounds[2] << bounds[3] << bounds[4] << bounds[5];

}

