#ifndef __vtkCmbLayeredConeSource_h
#define __vtkCmbLayeredConeSource_h

#include <vector>

#include <vtkMultiBlockDataSetAlgorithm.h>

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
  double GetTopRadius(int layer);

  void SetBaseRadius(int layer, double radius);
  double GetBaseRadius(int layer);

  vtkSetMacro(Height, double);
  vtkGetMacro(Height, double);

  vtkSetMacro(Resolution, int);
  vtkGetMacro(Resolution, int);

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

  double Height;
  std::vector<double> BaseRadii;
  std::vector<double> TopRadii;
  double BaseCenter[3];
  double Direction[3];
  int Resolution;
  int GenerateNormals;

private:
  vtkCmbLayeredConeSource(const vtkCmbLayeredConeSource&);
  void operator=(const vtkCmbLayeredConeSource&);
};

#endif
