#include "Scene.h"

#define EPSILON 0.00001

using namespace glm;

vec3 computeNormal(vec3 v1, vec3 v2, vec3 v3) {
	vec3 const a = v1;
	vec3 const b = v2;
	vec3 const c = v3;
	return normalize(cross(c - a, b - a));
}

bool box_intersection(vec3 ray_pos, vec3 inv_dir, vec3 boxMin, vec3 boxMax, float& tmin, float& tmax) {
	float lo = inv_dir.x*(boxMin.x - ray_pos.x);
	float hi = inv_dir.x*(boxMax.x - ray_pos.x);
	tmin = min(lo, hi);
	tmax = max(lo, hi);

	float lo1 = inv_dir.y*(boxMin.y - ray_pos.y);
	float hi1 = inv_dir.y*(boxMax.y - ray_pos.y);
	tmin = max(tmin, min(lo1, hi1));
	tmax = min(tmax, max(lo1, hi1));

	float lo2 = inv_dir.z*(boxMin.z - ray_pos.z);
	float hi2 = inv_dir.z*(boxMax.z - ray_pos.z);
	tmin = max(tmin, min(lo2, hi2));
	tmax = min(tmax, max(lo2, hi2));

	return (tmin <= tmax) && (tmax > 0.0f);
}

float FresnelReflectance(float IncidentCosine, float Rl) {
	float ci2 = IncidentCosine * IncidentCosine; 
	float si2 = 1.0f - ci2;
	float si4 = si2 * si2; 
	float a = ci2 + Rl * Rl - 1.0f; 
	float sqa = 2.0f * sqrtf(a) * IncidentCosine;
	float b = ci2 + a;
	float c = ci2 * a + si4;
	float d = sqa * si2;
	return (b - sqa) / (b + sqa) * (1.0f - d / (c + d));
}

bool triangle_intersection(vec3 V1, vec3 V2, vec3 V3, vec3 O, vec3 D, float& out) {
	//Find vectors for two edges sharing V1
	vec3 e1 = V2 - V1;
	vec3 e2 = V3 - V1;
	//Begin calculating determinant - also used to calculate u parameter
	vec3 P = cross(D, e2);
	//if determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
	float det = dot(e1, P);
	//NOT CULLING
	if (det > -EPSILON && det < EPSILON) return false;
	float inv_det = 1.0f / det;

	//calculate distance from V1 to ray origin
	vec3 T = O - V1;

	//Calculate u parameter and test bound
	float u = dot(T, P) * inv_det;
	//The intersection lies outside of the triangle
	if (u < 0.0f || u > 1.0f) return false;

	//Prepare to test v parameter
	vec3 Q = cross (T, e1);

	//Calculate V parameter and test bound
	float v = dot(D, Q) * inv_det;
	//The intersection lies outside of the triangle
	if (v < 0.0f || u + v  > 1.0f) return false;

	float t = dot(e2, Q) * inv_det;

	if (t > EPSILON) { //ray intersection
		out = t;
		return true;
	}

	// No hit, no win
	return 0;
}

bool sphere_intersection(vec3 ray_pos, vec3 ray_dir, vec3 spos, float r, float& tResult) {
	vec3 k = ray_pos - spos;
	float b = dot(k, ray_dir);
	float c = dot(k, k) - r*r;
	float d = b*b - c;

	if (d >= 0)
	{
		float sqrtfd = sqrtf(d);
		float t1 = -b + sqrtfd;
		float t2 = -b - sqrtfd;

		float min_t = min(t1, t2);
		float max_t = max(t1, t2);

		float t = (min_t >= 0) ? min_t : max_t;
		tResult = t;
		return (t > 0);
	}
	return false;
}

//—оздаем объекты CornellBox
void CScene::CreateScene() {
	lamp = Light(m_param->Light_Pos, m_param->Light_Dir); //«адаем параметры фонар€
	objects.push_back(new Sphere());
	objects.push_back(new Parallelepiped());
	objects.push_back(new PhysLamp(lamp.LightPos, lamp.LightDir));
	objects.push_back(new Box());
	TraceLight();
}

SRay CScene::OpticSystem(SRay ray, float& energy_remain, int i) {
	float t = 0;
	vec3 near_sphere_cen = lamp.LightPos + normalize(lamp.LightDir) * lamp.lens[i].C1;  ///R1  )  <----фонарь
	vec3 far_sphere_cen = lamp.LightPos - normalize(lamp.LightDir) * lamp.lens[i].C2;   ///R2 (   <----фонарь
	if (sphere_intersection(ray.m_start, ray.m_dir, near_sphere_cen, lamp.lens[i].R1, t)) { //пересечение с внешней сферой
		vec3 pos = ray.m_start + ray.m_dir * t;
		if (length(pos - far_sphere_cen) - lamp.lens[i].R2 < EPSILON) {                 //попадание на линзу (во внутреннюю сферу)
			vec3 normal = normalize(pos - near_sphere_cen);

			//потер€ энергии
			float IncidentCosine = abs(dot(ray.m_dir, normal));                         
			energy_remain *= 1.0f - FresnelReflectance(IncidentCosine, lamp.lens[i].Rl);

			//направление после искривлени€
			ray.m_dir += (sqrt((lamp.lens[i].Rl * lamp.lens[i].Rl - 1) / pow(dot(ray.m_dir, normal), 2) + 1) - 1) * dot(ray.m_dir, normal) * normal;
			if (length(ray.m_dir) < EPSILON)
				return SRay(vec3(-10, -10, -10), vec3(0, 0, 0));
			ray.m_dir = normalize(ray.m_dir);
			if (i == lamp.lens.size() - 1) return SRay(pos, ray.m_dir);
			else return OpticSystem(SRay(pos, ray.m_dir), energy_remain, i + 1);
		}
		else return SRay(vec3(-10.0f, -10.0f, -10.0f), vec3(0, 0, 0));
	}
	else return SRay(vec3(-10.0f, -10.0f, -10.0f), vec3(0, 0, 0));
}

void CScene::TraceLight() {
#pragma omp parallel if (m_param->OMP)
#pragma omp for
	for (int n = 0; n < 50; n++) {               //точечные источники света, в фонаре же он не один
		vec3 variance = sphericalRand(0.01f);    //их смещени€ относительно позиции фонар€
		for (float alpha = -15.0f; alpha < 0.0f; alpha += 0.3) {
			for (float betta = 0; betta < 360; betta += 1)
			{
				vec3 direction = lamp.LightDir;
				direction = rotate(direction, alpha, vec3(0, -direction.z, direction.y));
				direction = rotate(direction, betta, lamp.LightDir);

				//printf("%.5f %.5f %.5f\n", direction.x, direction.y, direction.z);

				float energy_remain = 1.0f;
				SRay ray;

				if (m_param->OpticSystem) ray = OpticSystem(SRay(lamp.LightPos + variance, normalize(direction)), energy_remain, 0);
				else ray = SRay(lamp.LightPos + variance, normalize(direction));
				if (length(ray.m_start - vec3(-10.0f, -10.0f, -10.0f)) < 1) continue;

				bool survive = true;
				for (int i = 0; i < objects.size() - 2; i++) {              //не погиб ли луч света на пути к полу
					vec3 pos = objects[i]->ObjectHit(ray, NULL);
					if (length(pos - vec3(-10.0f, -10.0f, -10.0f)) > EPSILON) {
						survive = false; break;
					}
				}

				if (survive) {
					vec3 pos = objects.back()->ObjectHit(ray, NULL);            //п€тно на полу
					if (length(pos - vec3(-10.0f, -10.0f, -10.0f)) < 1 || (pos.y + 1.0f) > EPSILON) continue;
					objects.back()->FillPhotonMap(pos, energy_remain);
				}
			}
		}
	}
}

Lens::Lens(float rl, float c1, float c2, float r1, float r2) {
    Rl = rl; R1 = r1; R2 = r2; C1 = c1; C2 = c2;
}

Light::Light(vec3 position, vec3 direction) {
	LightPos = position;
	LightDir = direction;
	lens.push_back(Lens(1.7, 2.2, 1.8, 2, 2.001));
	lens.push_back(Lens(1.5, 2.5, 2, 2.27, 2.25));
	ambient = 0.2;
}

//‘ункции класса стенок
Box::Box() {
	Points = {
		vec3(-1, -1, -1),
		vec3(1, 1, 1),
	};
	for (size_t i = 0; i < 401; ++i)
	{
		std::vector<float> temp;
		for (size_t j = 0; j < 401; ++j)
			temp.push_back(0);
		PhotonMap.push_back(temp);
	}
	step = 2.0f / 400;
	intensity = 0.005f;
}

vec3 Box::ObjectHit(SRay ray, glm::vec3* normal) {
	float tmin = 0;
	float tmax = 0;
	if (box_intersection(ray.m_start, 1.0f / ray.m_dir, Points[0], Points[1], tmin, tmax)) {
		vec3 pos = ray.m_start + ray.m_dir * tmax;
#pragma omp critical     //нельз€ высчитывать новые нормали, пока мы не запомнили старую
		{
			if (pos.x + 1 < EPSILON && pos.x + 1 > -EPSILON) curNormal = vec3(1, 0, 0);
			else if (pos.y - 1 < EPSILON && pos.y - 1 > -EPSILON) curNormal = vec3(0, -1, 0);
			else if (pos.x - 1 < EPSILON && pos.x - 1 > -EPSILON) curNormal = vec3(-1, 0, 0);
			else if (pos.y + 1 < EPSILON && pos.y + 1 > -EPSILON) curNormal = vec3(0, 1, 0);
			else curNormal = vec3(0, 0, -1);
			if (normal != NULL) *normal = curNormal;
		}
		return pos;
	}
	return vec3(-10.0f, -10.0f, -10.0f);
}

vec3 Box::GetColor(vec3 pos) {
	if ((pos.x + 1.0f) < EPSILON && (pos.x + 1.0f) > -EPSILON) return vec3(1, 0, 0);
	if ((pos.x - 1.0f) < EPSILON && (pos.x - 1.0f) > -EPSILON) return vec3(0, 1, 0);
	if ((pos.y - 1.0f) < EPSILON && (pos.y - 1.0f) > -EPSILON) return vec3(0.9, 0.9, 0.9);
	if ((pos.y + 1.0f) < EPSILON && (pos.y + 1.0f) > -EPSILON) return vec3(0.85, 0.85, 0.85);
	if ((pos.z - 1.0f) < EPSILON && (pos.z - 1.0f) > -EPSILON) return vec3(0.8, 0.8, 0.8);
	printf("%f\n", pos.z); system("pause");
	return vec3(1, 1, 0);
}

void Box::FillPhotonMap(vec3 pos, float energy_remain) {
    int i = (pos.x + 1) / step;
	int j = (pos.z + 1) / step;
	PhotonMap[i][j] += intensity * energy_remain;
}

float Box::GetPhotonMap(vec3 pos) {
	if (pos.y + 1.0 > EPSILON) return 0;
	int i = (pos.x + 1.0f) / step;
	int j = (pos.z + 1.0f) / step;
	float count_pixels = 0.0f;
	float energy = 0.0f;

	for (int a = -1; a < 2; a++)
	    for (int b = -1; b < 2; b++)
		    if (i + a >= 0 && j + b >= 0 && i + a <= 400 && j + b <= 400) {
			    energy += PhotonMap[i + a][j + b];
			    count_pixels += 1.0f;
		    }

	return energy / count_pixels;
}

Parallelepiped::Parallelepiped() {
	Points = {
		vec3(0, 0, 0),//a
		vec3(0, 2.7, 0),//a1
		vec3(0, 2.7, 1),//b1
		vec3(0, 0, 0),//a
		vec3(0, 2.7, 1),//b1
		vec3(0, 0, 1),//b

		vec3(0, 0, 1),//b
		vec3(0, 2.7, 1),//b1
		vec3(1, 2.7, 1),//c1
		vec3(1, 0, 1), //c
		vec3(0, 0, 1),//b
		vec3(1, 2.7, 1),//c1

		vec3(0, 2.7, 1),//b1
		vec3(0, 2.7, 0),//a1
		vec3(1, 2.7, 1),//c1
		vec3(0, 2.7, 0),//a1
		vec3(1, 2.7, 0),//d1
		vec3(1, 2.7, 1),//c1

		vec3(0, 0, 0),//a
		vec3(0, 0, 1),//b
		vec3(1, 0, 1), //c
		vec3(1, 0, 0), //d
		vec3(0, 0, 0),//a
		vec3(1, 0, 1), //c

		vec3(1, 2.7, 0),//d1
		vec3(1, 0, 0), //d
		vec3(1, 2.7, 1),//c1
		vec3(1, 0, 0), //d
		vec3(1, 0, 1), //c
		vec3(1, 2.7, 1),//c1

		vec3(0, 2.7, 0),//a1
		vec3(0, 0, 0),//a
		vec3(1, 2.7, 0),//d1
		vec3(0, 0, 0),//a
		vec3(1, 0, 0),//d
		vec3(1, 2.7, 0),//d1
	};

	for (int i = 0; i < 36; i++) {
		Points[i] *= 0.25f;
		Points[i] = rotateY(Points[i], 15.0f);
		Points[i] += vec3(-0.8, -1.0, 0.4);
	}
}

vec3 Parallelepiped::ObjectHit(SRay ray, glm::vec3* normal) {
	float t = -1;
	float tmin = -1;
	for (int i = 0; i < 12; i++) {
		if (triangle_intersection(Points[3 * i], Points[3 * i + 1], Points[3 * i + 2], ray.m_start, ray.m_dir, t)) {
			if (tmin < 0 || tmin - t > EPSILON) {
				tmin = t;
#pragma omp critical     //нельз€ высчитывать новые нормали, пока мы не запомнили старую
				{
					curNormal = computeNormal(Points[3 * i], Points[3 * i + 1], Points[3 * i + 2]);
					if (normal != NULL) *normal = curNormal;
				}
			}
		}
	}
	if (tmin > 0) return ray.m_start + ray.m_dir * tmin;
	return vec3(-10.0f, -10.0f, -10.0f);
}

vec3 Parallelepiped::GetColor(vec3 pos) {
	return vec3(0, 1, 0);
}

Sphere::Sphere() {
	Points = { vec3(-0.27, -0.8, 0.35) };
	radius = 0.2f;
}

vec3 Sphere::ObjectHit(SRay ray, glm::vec3* normal) {
	float t = 0;
	if (sphere_intersection(ray.m_start, ray.m_dir, Points[0], radius, t)) {
#pragma omp critical     //нельз€ высчитывать новые нормали, пока мы не запомнили старую
		{
			curNormal = normalize(ray.m_start + ray.m_dir * t - Points[0]);
			if (normal != NULL) *normal = curNormal;
		}
		return ray.m_start + ray.m_dir * t;
	}
	return vec3(-10.0f, -10.0f, -10.0f);
}

vec3 Sphere::GetColor(vec3 pos) {
	return vec3(1, 0, 1);
}

//‘ункции класса стенок
PhysLamp::PhysLamp(vec3 LightPos, vec3 LightDir) {
	Points = {
		vec3(0, 0, 0),//a
		vec3(0, 2, 0),//a1
		vec3(0, 2, 1),//b1
		vec3(0, 0, 0),//a
		vec3(0, 2, 1),//b1
		vec3(0, 0, 1),//b

		vec3(0, 0, 1),//b
		vec3(0, 2, 1),//b1
		vec3(1, 2, 1),//c1
		vec3(1, 0, 1), //c
		vec3(0, 0, 1),//b
		vec3(1, 2, 1),//c1

		vec3(0, 2, 1),//b1
		vec3(0, 2, 0),//a1
		vec3(1, 2, 1),//c1
		vec3(0, 2, 0),//a1
		vec3(1, 2, 0),//d1
		vec3(1, 2, 1),//c1

		vec3(0, 0, 0),//a
		vec3(0, 0, 1),//b
		vec3(1, 0, 1), //c
		vec3(1, 0, 0), //d
		vec3(0, 0, 0),//a
		vec3(1, 0, 1), //c

		vec3(1, 2, 0),//d1
		vec3(1, 0, 0), //d
		vec3(1, 2, 1),//c1
		vec3(1, 0, 0), //d
		vec3(1, 0, 1), //c
		vec3(1, 2, 1),//c1

		vec3(0, 2, 0),//a1
		vec3(0, 0, 0),//a
		vec3(1, 2, 0),//d1
		vec3(0, 0, 0),//a
		vec3(1, 0, 0),//d
		vec3(1, 2, 0),//d1
	};
	for (int i = 0; i < 36; i++) {
		Points[i] = rotate(Points[i], angle(vec3(0, -1, 0), normalize(LightDir)), normalize(vec3(-LightDir.z, 0, LightDir.x)));
		Points[i] *= 0.05;
		Points[i] += LightPos - vec3(0.025, 0.025, 0.025);
	}
	LightDirection = normalize(LightDir);
}

vec3 PhysLamp::ObjectHit(SRay ray, glm::vec3* normal) {
	float t = -1;
	float tmin = -1;
	for (int i = 0; i < 12; i++) {
		if (triangle_intersection(Points[3 * i], Points[3 * i + 1], Points[3 * i + 2], ray.m_start, ray.m_dir, t)) {
			if (tmin < 0 || tmin - t > EPSILON) {
				tmin = t;
#pragma omp critical     //нельз€ высчитывать новые нормали, пока мы не запомнили старую
				{
					curNormal = computeNormal(Points[3 * i], Points[3 * i + 1], Points[3 * i + 2]);
					if (normal != NULL) *normal = curNormal;
				}
			}
		}
	}
	if (tmin > 0) return ray.m_start + ray.m_dir * tmin;
	return vec3(-10.0f, -10.0f, -10.0f);
}

vec3 PhysLamp::GetColor(vec3 pos) {
	if (dot(curNormal, LightDirection) - 1 < EPSILON && dot(curNormal, LightDirection) - 1 > -EPSILON) return vec3(1, 1, 1);
	else return vec3(0, 0, 0);
}