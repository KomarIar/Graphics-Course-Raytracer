#include "Tracer.h"

using namespace glm;

#define EPSILON 0.00001

SRay CTracer::MakeRay(glm::uvec2 pixelPos, float xvar, float yvar) {
	return SRay(m_camera.m_pos, normalize(m_camera.m_forward + m_camera.m_right * ((pixelPos.x + xvar + 0.5f) / m_camera.m_resolution.x - 0.5f) + 
		                                             m_camera.m_up * ((pixelPos.y + yvar + 0.5f) / m_camera.m_resolution.y - 0.5f)));
}

glm::vec3 CTracer::TraceRay(SRay ray) {
    vec3 color(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < m_pScene->objects.size(); i++) {
		vec3 pos; vec3 Normal;
		pos = m_pScene->objects[i]->ObjectHit(ray, &Normal);
		if (length(pos - vec3(-10.0f, -10.0f, -10.0f)) < EPSILON) continue; //проверка попал ли луч в объект
		color = m_pScene->objects[i]->GetColor(pos);
		if (i == m_pScene->objects.size() - 2) return color; //попадание в фонарик

		if (m_param->Shadow) {                               //тени
			SRay shadow_ray(pos, normalize(m_pScene->lamp.LightPos - pos));
			for (int j = 0; j < m_pScene->objects.size() - 2; j++) {
				if (i == j) continue;
				vec3 shadow_pos = m_pScene->objects[j]->ObjectHit(shadow_ray, NULL);
				if (length(shadow_pos - vec3(-10.0f, -10.0f, -10.0f)) > EPSILON) {
					return color * m_pScene->lamp.ambient;
				}
			}
		}

		vec3 LightVector = normalize(m_pScene->lamp.LightPos - pos);
		vec3 ViewVector = normalize(m_camera.m_pos - pos);
		vec3 Reflect = reflect(LightVector, Normal);
		float Away = length(m_pScene->lamp.LightPos - pos);

		float angle = fmax(0.0f, dot(normalize(m_pScene->lamp.LightDir), -LightVector)); //угол между направлением фонарика и точкой
		if (angle < 0.55f) angle *= angle;                                //более "параболическое" освещение (источник внутри фонаря)
		float Diffuse = fmax(0.0f, dot(Normal, LightVector)) * angle * (1.0f - Away / sqrt(12.0f));

		if (pos.y + 1.0 < EPSILON) {
			color *= m_pScene->objects[i]->GetPhotonMap(pos) + Diffuse + m_pScene->lamp.ambient;
			return color;
		}
		float Specular = pow(fmax(0.0f, dot(-ViewVector, Reflect)), 30.0f);
		if (angle < 0.55f) Specular = 0.0f;
		color *= Specular + Diffuse + m_pScene->lamp.ambient;
		return color;
	}
    return color;
}

void CTracer::RenderImage(int xRes, int yRes)
{
    // Rendering
    m_camera.m_resolution = uvec2(xRes, yRes);
    m_camera.m_pixels.resize(xRes * yRes);

    //стандартная камера
    m_camera.m_pos = m_param->Camera_Pos;
    m_camera.m_forward = normalize(vec3(0, 0, 1)) * 2.0f;                   //длина вектора равна расстоянию до плоскости проекции
    m_camera.m_right = normalize(vec3(1, 0, 0)) * 1.5f;                     //длина векторов равна соответствующему размеру плоскости проекции
    m_camera.m_up = normalize(vec3(0, 1, 0)) * (float (yRes) / float (xRes)) * 1.5f; //длина векторов равна соответствующему размеру плоскости проекции
  
    m_camera.m_forward = rotateY(m_camera.m_forward, m_param->AngleY);
    m_camera.m_right = rotateY(m_camera.m_right, m_param->AngleY);
    m_camera.m_forward = rotateX(m_camera.m_forward, m_param->AngleX);
    m_camera.m_up = rotateX(m_camera.m_up, m_param->AngleX);

  /*камера 2
  m_camera.m_pos = vec3(-0.5, 0, -3);
  m_camera.m_forward = normalize(vec3(0.3, 0, 1)) * 2.0f; //длина вектора равна расстоянию до плоскости проекции
  m_camera.m_right = normalize(vec3(1, 0, -0.3)) * 2.0f;  //длина векторов равна соответствующему размеру плоскости проекции
  m_camera.m_up = normalize(vec3(0, 1, 0)) * 2.0f;        //длина векторов равна соответствующему размеру плоскости проекции
  */
	int i = 0;
	int j = 0;
	if (m_param->AA) {
#pragma omp parallel if (m_param->OMP)
		{
			for (i = 0; i < yRes; i++) {
#pragma omp for
				for (j = 0; j < xRes; j++) {
					SRay ray = MakeRay(uvec2(j, i), -0.25f, -0.25f);
					m_camera.m_pixels[i * xRes + j] += TraceRay(ray);
					ray = MakeRay(uvec2(j, i), 0.25f, -0.25f);
					m_camera.m_pixels[i * xRes + j] += TraceRay(ray);
					ray = MakeRay(uvec2(j, i), -0.25f, 0.25f);
					m_camera.m_pixels[i * xRes + j] += TraceRay(ray);
					ray = MakeRay(uvec2(j, i), 0.25f, 0.25f);
					m_camera.m_pixels[i * xRes + j] += TraceRay(ray);
					m_camera.m_pixels[i * xRes + j] *= 0.25f;
				}
			}
		}
	}
	else {
#pragma omp parallel if (m_param->OMP)
		{
			for (int i = 0; i < yRes; i++){
#pragma omp for 
				for (int j = 0; j < xRes; j++) {
					SRay ray = MakeRay(uvec2(j, i), 0.0f, 0.0f);
					m_camera.m_pixels[i * xRes + j] = TraceRay(ray);
				}
			}
	    }
	}
}

void CTracer::SaveImageToFile(std::string fileName)
{
    CImage image;

    int width = m_camera.m_resolution.x;
    int height = m_camera.m_resolution.y;

    image.Create(width, height, 24);
    
	int pitch = image.GetPitch();
	unsigned char* imageBuffer = (unsigned char*)image.GetBits();

	if (pitch < 0) {
		imageBuffer += pitch * (height - 1);
		pitch =- pitch;
	}

	int i, j;
	int imageDisplacement = 0;
	int textureDisplacement = 0;

	for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++)
        {
            vec3 color = m_camera.m_pixels[textureDisplacement + j];

            imageBuffer[imageDisplacement + j * 3] = clamp(color.b, 0.0f, 1.0f) * 255.0f;
            imageBuffer[imageDisplacement + j * 3 + 1] = clamp(color.g, 0.0f, 1.0f) * 255.0f;
            imageBuffer[imageDisplacement + j * 3 + 2] = clamp(color.r, 0.0f, 1.0f) * 255.0f;
        }

		imageDisplacement += pitch;
		textureDisplacement += width;
	}

    image.Save(fileName.c_str());
	image.Destroy();
}

CImage* CTracer::LoadImageFromFile(std::string fileName)
{
  CImage* pImage = new CImage;

  if(SUCCEEDED(pImage->Load(fileName.c_str())))
    return pImage;
  else
  {
    delete pImage;
    return NULL;
  }
}