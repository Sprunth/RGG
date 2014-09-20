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
#include <QDebug>
#include <cmath>

#include "cmbNucDefaults.h"
#include "vtkCmbLayeredConeSource.h"

typedef cmbNucRender::point point;
typedef cmbNucRender::GeoToPoints GeoToPoints;
typedef cmbNucRender::key key;

int INITIAL_KEY_TEST=1;
const int PinCellResolution = 18;

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
    double startX = assy->AssyDuct.getDuct(0)->x;
    double startY = assy->AssyDuct.getDuct(0)->y;
    double outerDuctWidth = assy->AssyDuct.getDuct(0)->thickness[1];
    double outerDuctHeight = assy->AssyDuct.getDuct(0)->thickness[0];

    int subType = lat.GetGeometrySubType();
    if( (subType & ANGLE_360) && lat.Grid.size()>=1 )
    {
      double odc = outerDuctHeight*lat.Grid.size();
      double tmp = odc - outerDuctHeight;
      double t2 = tmp*0.5;
      double ty = std::sqrt(tmp*tmp-t2*t2);
      extraYTrans = -ty;
      extraXTrans = odc;
    }

    for(size_t i = 0; i < lat.Grid.size(); i++)
    {
      const std::vector<Lattice::LatticeCell> &row = lat.Grid[i];

      for(size_t j = 0; j < row.size(); j++)
      {
        if(!row[j].isBlank())
        {
          const std::string &type = row[j].label;
          cmbNucAssembly* assembly = input->GetAssembly(type);
          if(assembly == NULL) continue;

          Duct *hexDuct = assembly->AssyDuct.getDuct(0);
          double layerCorners[6][2], hexDiameter, layerRadius;
          hexDiameter = hexDuct->thickness[0];

          // For hex geometry type, figure out the six corners first
          if(i>0)
          {
            layerRadius = hexDiameter * i;
            for(int c = 0; c < 6; c++)
            {
              // Needs a little more thinking on math here?
              layerCorners[c][0] = layerRadius * cmbNucCore::CosSinAngles[c][0];
              layerCorners[c][1] = layerRadius * cmbNucCore::CosSinAngles[c][1];
            }
          }

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
            cornerIdx = j / i;
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
    Duct *hexDuct = input->AssyDuct.getDuct(0);
    double layerRadius;
    Lattice & lat = input->getLattice();

    double overallDx = 0;
    for(size_t i = 0; i < lat.Grid.size(); i++)
    {
      double overallDy = 0;

      const std::vector<Lattice::LatticeCell> &row = lat.Grid[i];
      for(size_t j = 0; j < row.size(); j++)
      {
        if(!row[j].isBlank())
        {
          const std::string &type = row[j].label;
          PinCell* pincell = input->GetPinCell(type);
          if(pincell && (pincell->GetNumberOfParts())>0)
          {
            double pinDistFromCenter = pincell->pitchX * (i);
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

    for(size_t i = 0; i < lat.Grid.size(); i++)
    {
      const std::vector<Lattice::LatticeCell> &row = lat.Grid[i];
      for(size_t j = 0; j < row.size(); j++)
      {
        if(!row[j].isBlank())
        {
          input->calculateRectPt(i, j, currentLaticePoint);
          idToPoint[row[j].label].push_back(point( currentLaticePoint[0],
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

  static void createGeo(PinCell* pincell, bool isHex,
                        std::map<key,GeoToPoints> & geometry)
  {
    if(pincell == NULL) return;
    size_t numParts = pincell->GetNumberOfParts();

    for(size_t j = 0; j < numParts; ++j)
    {
      PinSubPart* part = pincell->GetPart(j);
      vtkSmartPointer<vtkCmbLayeredConeSource> manager = cmbNucRender::CreateLayerManager(pincell, isHex, j);
      if(manager == NULL) continue;
      //inner cylinder/frustum
      double thickness[] = {manager->GetTopThickness(0), manager->GetBaseThickness(0)};
      if(thickness[0] == thickness[1])
      {
        key k;
        k.type = key::Cylinder;
        k.sides = manager->GetResolution(0);
        if(geometry.find(k) == geometry.end())
        {
          geometry[k].geo = manager->CreateUnitLayer(0);
        }
        GeoToPoints & geo = geometry[k];
        point loc;
        manager->GetBaseCenter(loc.xyz);
        double h = manager->GetHeight();
        cmbNucRender::scale scale(thickness[0],thickness[0],h);
        geo.points.push_back(GeoToPoints::data(loc, point(), part->GetMaterial(0), scale));
      }
      else
      {
        //todo frustum
      }
      for(int i = 0; i < manager->GetNumberOfLayers(); ++i)
      {
        //todo annuluses
      }
      
    }
  }

  static void createGeo(cmbNucAssembly * input, std::map<key,GeoToPoints> & geometry)
  {
    std::map< std::string, std::vector<point> > idToPoint;
    double extraXTrans = 0;
    double extraYTrans = 0;
    calculatePoints(input, extraXTrans, extraYTrans, idToPoint);
    //For now support only one rotation.  More work will need to be done for a full xfrom support
    cmbNucAssembly::Transform * xform = NULL;
    for(unsigned int i = 0; i < input->getNumberOfTransforms(); ++i)
    {
      cmbNucAssembly::Transform* tmpxf = input->getTransform(i);
      if(tmpxf != NULL && tmpxf->getLabel() == "Rotate")
      {
        xform = tmpxf;
        break;
      }
    }
    point xformR;
    if(xform != NULL)
    {
      xformR.xyz[xform->getAxis()] = xform->getValue();
    }
    //Duct
    vtkMultiBlockDataSet* dataSet = cmbNucAssembly::CreateDuctCellMultiBlock(&(input->AssyDuct), input->IsHexType());
    for(int block=0; block<dataSet->GetNumberOfBlocks(); block++)
    {
      vtkMultiBlockDataSet* sectionBlock = vtkMultiBlockDataSet::SafeDownCast(dataSet->GetBlock(block));
      Duct* duct = input->AssyDuct.getDuct(block);
      if(!sectionBlock)
      {
        continue;
      }
      for(int layer=0; layer<sectionBlock->GetNumberOfBlocks(); layer++)
      {

        vtkPolyData* polyBlock = vtkPolyData::SafeDownCast(sectionBlock->GetBlock(layer));
        if(!polyBlock)
        {
          continue;
        }
        cmbNucMaterial * mat = duct->getMaterial(layer);
        GeoToPoints geo;
        geo.geo = vtkSmartPointer<vtkPolyData>::New();
        geo.geo->DeepCopy(polyBlock);
        geo.points.resize(1, GeoToPoints::data(point(), xformR, mat));

        key k;
        k.type = key::AnnulusCylinder;
        k.sides = INITIAL_KEY_TEST++;

        geometry[k] = geo;
      }
    }
    if(input->IsHexType())
    {
      xformR.xyz[2] +=30; //for cell material
    }
    //pins
    for(std::map< std::string, std::vector<point> >::const_iterator iter = idToPoint.begin();
        iter != idToPoint.end(); ++iter)
    {
      std::map<key, GeoToPoints> tmpGeo;
      //prepopulate the keys
      for(std::map<key, GeoToPoints>::const_iterator i = geometry.begin(); i != geometry.end(); ++i)
      {
        tmpGeo[i->first] = GeoToPoints();
      }

      createGeo(input->GetPinCell(iter->first), input->IsHexType(), tmpGeo);

      std::vector<point> const& points = iter->second;
      mergeGeometry(tmpGeo, points, extraXTrans, extraYTrans, geometry, xform, xformR);
      //start of delete
#if 0
      if(!pincell) continue;
      vtkMultiBlockDataSet* pincellDataBlock = cmbNucAssembly::CreatePinCellMultiBlock(pincell, input->IsHexType());

      for(int block=0; block<pincellDataBlock->GetNumberOfBlocks(); block++)
      {
        vtkMultiBlockDataSet* sectionBlock = vtkMultiBlockDataSet::SafeDownCast(pincellDataBlock->GetBlock(block));
        if(!sectionBlock)
        {
          continue;
        }
        for(int layer=0; layer<sectionBlock->GetNumberOfBlocks(); layer++)
        {
          vtkPolyData* polyBlock = vtkPolyData::SafeDownCast(sectionBlock->GetBlock(layer));
          if(!polyBlock)
          {
            continue;
          }
          cmbNucMaterial * mat = pincell->GetPart(block)->GetMaterial(layer);
          if(mat == NULL && pincell->cellMaterialSet())
          {
            mat = pincell->getCellMaterial();
          }
          if(mat == NULL) continue;
          GeoToPoints geo;
          geo.geo = vtkSmartPointer<vtkPolyData>::New();
          geo.geo->DeepCopy(polyBlock);
          for(unsigned int i = 0; i < points.size(); ++i)
          {
            point pt = points[i];
            pt.xyz[0] += extraXTrans;
            pt.xyz[1] += extraYTrans;
            if(xform != NULL)
            {
              double tp[3];
              xform->apply(pt.xyz, tp);
              pt.xyz[0] = tp[0];
              pt.xyz[1] = tp[1];
              pt.xyz[2] = tp[2];
            }
            geo.points.push_back(GeoToPoints::data(pt, xformR, mat));
          }
          key k;
          k.type = key::AnnulusCylinder;
          k.sides = INITIAL_KEY_TEST++;

          geometry[k] = geo;
        }
      }
#endif
    }
  }

  static void createGeo(cmbNucCore * input, std::map<key, GeoToPoints> & geometry)
  {
    std::map< std::string, std::vector<point> > idToPoint;
    double extraXTrans;
    double extraYTrans;
    calculatePoints(input, extraXTrans, extraYTrans, idToPoint);
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
        tmpGeo[i->first] = GeoToPoints();
      }

      createGeo(assembly, tmpGeo);
      std::vector<point> const& points = iter->second;
      mergeGeometry(tmpGeo, points, extraXTrans, extraYTrans, geometry);
    }
  }

  static void mergeGeometry(std::map<key, GeoToPoints> const& in,
                            std::vector<point> const& points,
                            double extraXTrans, double extraYTrans,
                            std::map<key, GeoToPoints> & geometry,
                            cmbNucAssembly::Transform* xform = NULL,
                            point xformR = point())
  {
    for(std::map<key, GeoToPoints>::const_iterator i = in.begin(); i != in.end(); ++i)
    {
      GeoToPoints const& tmpGeoI = i->second;
      if(geometry.find(i->first) == geometry.end())
      {
        GeoToPoints newGeo;
        newGeo.geo = tmpGeoI.geo;
        geometry[i->first] = newGeo;
      }
      GeoToPoints &newGeo = geometry[i->first];

      unsigned int at = newGeo.points.size();
      newGeo.points.resize(newGeo.points.size() + points.size()*tmpGeoI.points.size());
      for(unsigned int j = 0; j<points.size(); ++j)
      {
        point const& apt = points[j];
        for(unsigned int k = 0; k < tmpGeoI.points.size(); ++k)
        {
          GeoToPoints::data const& pt = tmpGeoI.points[k];
          point tranpt = pt.pt;
          tranpt.xyz[0] += apt.xyz[0] + extraXTrans;
          tranpt.xyz[1] += apt.xyz[1] + extraYTrans;
          if(xform != NULL)
          {
            double tp[3];
            xform->apply(tranpt.xyz, tp);
            tranpt.xyz[0] = tp[0];
            tranpt.xyz[1] = tp[1];
            tranpt.xyz[2] = tp[2];
          }
          point rotation = pt.rotation;
          rotation.xyz[0] += xformR.xyz[0];
          rotation.xyz[1] += xformR.xyz[1];
          rotation.xyz[2] += xformR.xyz[2];

          newGeo.points[at] = GeoToPoints::data(tranpt,rotation, pt.material, pt.ptScale);
          ++at;
        }
      }
    }
  }
};

cmbNucRender::cmbNucRender()
 :/*PolyMapper(vtkSmartPointer<vtkCompositePolyDataMapper2>::New()),
  PolyActor(vtkSmartPointer<vtkActor>::New()),*/
  GlyphMapper(vtkSmartPointer<vtkGlyph3DMapper>::New()),
  GlyphActor(vtkSmartPointer<vtkActor>::New()),
  TransparentMapper(vtkSmartPointer<vtkGlyph3DMapper>::New()),
  TransparentActor(vtkSmartPointer<vtkActor>::New())
{
  /*this->PolyMapper->SetScalarVisibility(0);
  this->PolyActor->SetMapper(this->PolyMapper.GetPointer());
  this->PolyActor->GetProperty()->SetShading(1);
  this->PolyActor->GetProperty()->SetInterpolationToPhong();*/

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
  sendToGlyphMappers(geometry);
  QPointer<cmbNucDefaults> defaults = core->GetDefaults();
  double dw = 0, dh = 0;
  defaults->getDuctThickness(dw,dh);
  dw *= 0.5;
  dh *= 0.5;
  double h;
  defaults->getHeight(h);
  double xyz[3];
  BoundingBox.GetMinPoint(xyz[0],xyz[1],xyz[2]);
  xyz[0] -= dw;
  xyz[1] -= dh;
  xyz[2] = h;
  BoundingBox.AddPoint(xyz);
  BoundingBox.GetMaxPoint(xyz[0],xyz[1],xyz[2]);
  xyz[0] += dw;
  xyz[1] += dh;
  xyz[2] = h;
  BoundingBox.AddPoint(xyz);
}

void cmbNucRender::render(cmbNucAssembly * assy)
{
  std::map<key, GeoToPoints> geometry;
  cmbNucRenderHelper::createGeo(assy, geometry);
  sendToGlyphMappers(geometry);
  QPointer<cmbNucDefaults> defaults = assy->getDefaults();
  double dw = 0, dh = 0;
  defaults->getDuctThickness(dw,dh);
  dw *= 0.5;
  dh *= 0.5;
  double h;
  defaults->getHeight(h);
  double xyz[3];
  BoundingBox.GetMinPoint(xyz[0],xyz[1],xyz[2]);
  xyz[0] -= dw;
  xyz[1] -= dh;
  xyz[2] = h;
  BoundingBox.AddPoint(xyz);
  BoundingBox.GetMaxPoint(xyz[0],xyz[1],xyz[2]);
  xyz[0] += dw;
  xyz[1] += dh;
  xyz[2] = h;
  BoundingBox.AddPoint(xyz);

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
  BoundingBox.Reset();
  vtkSmartPointer<vtkPoints> polyPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> polyPointsTrans = vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkIntArray> polyScalers = vtkSmartPointer<vtkIntArray>::New();
  vtkSmartPointer<vtkIntArray> polyScalersTrans = vtkSmartPointer<vtkIntArray>::New();

  vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  vtkSmartPointer<vtkUnsignedCharArray> colorsTrans = vtkSmartPointer<vtkUnsignedCharArray>::New();

  vtkSmartPointer<vtkDoubleArray> rotationAngles = vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkDoubleArray> rotationAnglesTrans = vtkSmartPointer<vtkDoubleArray>::New();

  vtkSmartPointer<vtkDoubleArray> scale = vtkSmartPointer<vtkDoubleArray>::New();
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

  scale->SetNumberOfComponents(3);
  scale->SetName("Scale");
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
      vtkSmartPointer<vtkDoubleArray> tscale = scale;
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
  polydata->GetPointData()->AddArray(scale);
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

  //GlyphMapper->PrintSelf(std::cout, vtkIndent());

  BoundingBox.AddBounds(polydataTrans->GetBounds());
  BoundingBox.AddBounds(polydata->GetBounds());
}

vtkSmartPointer<vtkCmbLayeredConeSource> cmbNucRender::CreateLayerManager(PinCell* pincell, bool isHex, size_t j)
{
  PinSubPart* part = pincell->GetPart(j);
  
  vtkSmartPointer<vtkCmbLayeredConeSource> coneSource = vtkSmartPointer<vtkCmbLayeredConeSource>::New();
  coneSource->SetNumberOfLayers(pincell->GetNumberOfLayers() + (pincell->cellMaterialSet()?1:0));
  coneSource->SetBaseCenter(0, 0, part->z1);
  coneSource->SetHeight(part->z2 - part->z1);
  double lastR[2];

  for(int k = 0; k < pincell->GetNumberOfLayers(); k++)
  {
    lastR[0] = part->getRadius(k,Frustum::BOTTOM);
    lastR[1] = part->getRadius(k,Frustum::TOP);
    coneSource->SetBaseRadius(k, part->getRadius(k,Frustum::BOTTOM));
    coneSource->SetTopRadius(k, part->getRadius(k,Frustum::TOP));
    coneSource->SetResolution(k, PinCellResolution);
  }
  if(pincell->cellMaterialSet())
  {
    double r[] = {pincell->pitchX*0.5, pincell->pitchY*0.5};
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
