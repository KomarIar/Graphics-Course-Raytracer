#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtc/random.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "vector"
#include "Types.h"

bool triangle_intersection(glm::vec3 V1, glm::vec3 V2, glm::vec3 V3, glm::vec3 O, glm::vec3 D, float* out);
bool sphere_intersection(glm::vec3 ray_pos, glm::vec3 ray_dir, glm::vec3 spos, float r, float& tResult);

class Lens
{
public:
	float Rl;     //коэффициент преломления
	float C1, C2; //смещение центров сфер линзы
	float R1, R2; //их радиусы
	Lens(float rl, float c1, float c2, float r1, float r2);
};

class Light
{
public:
	glm::vec3 LightPos;
	glm::vec3 LightDir;
	std::vector<class Lens> lens;
	float ambient;
public:
	Light() {};
	Light(glm::vec3 LightPos, glm::vec3 LightDir);
};

class Object
{
public:
	std::vector<glm::vec3> Points;                  //Координаты точек, задающих объект сцены
	std::vector<std::vector<float>> PhotonMap;
	glm::vec3 curNormal;
public:
	virtual glm::vec3 ObjectHit(SRay ray, glm::vec3* normal) = 0;      //Возвращает координату попадания в объект, по-своему определен у каждого объекта
	virtual glm::vec3 GetColor(glm::vec3 pos) = 0;  //Возвращает цвет объекта в точке, по-своему определен у каждого объекта
	virtual void FillPhotonMap(glm::vec3 pos, float remain_energy) {};
	virtual float GetPhotonMap(glm::vec3 pos) { return 0; }
};

class PhysLamp : public Object
{
public:
	glm::vec3 LightDirection;
public:
	PhysLamp(glm::vec3 LightPos, glm::vec3 LightDir);
	glm::vec3 ObjectHit(SRay ray, glm::vec3* normal);
	glm::vec3 GetColor(glm::vec3 pos);
};

class Box : public Object //Стенки
{
public:
	float step;
	float intensity;
public:
	Box();
	glm::vec3 ObjectHit(SRay ray, glm::vec3* normal);
	glm::vec3 GetColor(glm::vec3 pos);
	void FillPhotonMap(glm::vec3 pos, float remain_energy);
	float GetPhotonMap(glm::vec3 pos);
};

class Parallelepiped : public Object //параллелепипед
{
public:
    Parallelepiped();
	glm::vec3 ObjectHit(SRay ray, glm::vec3* normal);
	glm::vec3 GetColor(glm::vec3 pos);
};

class Sphere : public Object //параллелепипед
{
public:
	float radius;
public:
	Sphere();
	glm::vec3 ObjectHit(SRay ray, glm::vec3* normal);
	glm::vec3 GetColor(glm::vec3 pos);
};

class CScene
{
public:
	std::vector<Object*> objects;
	Light lamp;
public:
	void TraceLight();
	void CreateScene();
	SRay OpticSystem(SRay ray, float& energy_remain, int i);
	Parameters* m_param;
};