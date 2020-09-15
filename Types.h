#pragma once

#include "glm/glm.hpp"
#include "vector"

struct Parameters
{
	bool AA = false;
	bool OpticSystem = true;
	bool Shadow = true;
	bool OMP = true;

	glm::vec3 Camera_Pos = glm::vec3(0.0f, 0.0f, -3.0f);
	float AngleY = 0.0f;
	float AngleX = 0.0f;

	glm::vec3 Light_Pos = glm::vec3(0.8f, 0.23f, 0.33f);
	glm::vec3 Light_Dir = glm::vec3(-0.5f, -1.0f, 0.0f);
};

struct SRay
{
    glm::vec3 m_start;
    glm::vec3 m_dir;

	SRay() {};

	SRay(glm::vec3 start, glm::vec3 dir) {
	    m_start = start;
	    m_dir = dir;
    }
};

struct SCamera
{
    glm::vec3 m_pos;          // Camera position and orientation
    glm::vec3 m_forward;      // Orthonormal basis
    glm::vec3 m_right;
    glm::vec3 m_up;

    glm::vec2 m_viewAngle;    // View angles, rad
    glm::uvec2 m_resolution;  // Image resolution: w, h

    std::vector<glm::vec3> m_pixels;  // Pixel array
};

struct SMesh
{
    std::vector<glm::vec3> m_vertices;  // vertex positions
    std::vector<glm::uvec3> m_triangles;  // vetrex indices
};