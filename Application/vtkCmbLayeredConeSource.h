#ifndef __vtkCmbLayeredConeSource_h
#define __vtkCmbLayeredConeSource_h

#include <vector>

#include <vtkMultiBlockDataSetAlgorithm.h>

class vtkPolyData;
class vtkPoints;

// Creates a cone with multiple layers. Each layer is a separate block
// in the multi-block output data-set so that individual layers property's
// can be modified with the composite poly-data mapper.
class vtkCmbLayeredConeSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCmbLayeredConeSource* New();
  vtkTypeMacro(vtkCmbLayeredConeSource, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent);

  void SetNumberOfLayers(int layers);
  int GetNumberOfLayers();

  void SetTopRadius(int layer, double radius);
  void SetTopRadius(int layer, double r1, double r2);
  double GetTopRadius(int layer);

  void SetBaseRadius(int layer, double radius);
  void SetBaseRadius(int layer, double r1, double r2);
  double GetBaseRadius(int layer);

  void SetResolution(int layer, int res);

  vtkSetMacro(Height, double);
  vtkGetMacro(Height, double);

  vtkSetVector3Macro(BaseCenter,double);
  vtkGetVectorMacro(BaseCenter,double,3);

  vtkSetVector3Macro(Direction,double);
  vtkGetVectorMacro(Direction,double,3);

  // Determines whether surface normals should be generated
  // On by default
  vtkSetMacro(GenerateNormals,int);
  vtkGetMacro(GenerateNormals,int);
  vtkBooleanMacro(GenerateNormals,int);

protected:
  vtkCmbLayeredConeSource();
  ~vtkCmbLayeredConeSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  //vtkMultiBlockDataSet * CreateLayer(double innerRadius, double outerRadius,
  //                                   int res1, int res2);

  vtkPolyData * CreateLayer(double h,
                            double * innerBottomR, double * outerBottomR,
                            double * innerTopR,    double * outerTopR,
                            int innerRes, int outerRes);

  double Height;
  struct radii
  {
    int Resolution;
    double BaseRadii[2];
    double TopRadii[2];
  };
  std::vector<radii> LayerRadii;
  double BaseCenter[3];
  double Direction[3];
  int Resolution;
  int GenerateNormals;

private:
  vtkCmbLayeredConeSource(const vtkCmbLayeredConeSource&);
  void operator=(const vtkCmbLayeredConeSource&);
};

#endif
