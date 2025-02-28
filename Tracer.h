#pragma once

#include "glm/glm.hpp"
#include "Types.h"
#include "Scene.h"
#include <omp.h>

#include "string"
#include "atlimage.h"

class CTracer
{
public:
  SRay MakeRay(glm::uvec2 pixelPos, float xvar, float yvar);  // Create ray for specified pixel
  glm::vec3 TraceRay(SRay ray); // Trace ray, compute its color
  void RenderImage(int xRes, int yRes);
  void SaveImageToFile(std::string fileName);
  CImage* LoadImageFromFile(std::string fileName);

public:
  SCamera m_camera;
  CScene* m_pScene;
  Parameters* m_param;
};