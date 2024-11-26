#pragma once

#include <stdint.h>

#ifdef WIN32
	#define CMB_API extern "C" __declspec(dllexport)
#else
	#define CMB_API extern "C"
#endif

enum cmb_BooleanType {
	CMB_UNION,
	CMB_INTERSECTION,
	CMB_DIFFERENCE,
	CMB_XOR,
};

struct cmb_InputMesh {
	uint32_t numVertices;
	uint32_t numTriangles;
	float* positions;
	uint32_t* indices;
};

struct cmb_CylinderInfo {
	float posX, posY, posZ;
	float dirX, dirY, dirZ;
	float radius;
	float halfHeight;
};

struct cmb_Result;

CMB_API cmb_Result* cmb_boolean(cmb_BooleanType type, cmb_InputMesh meshA, cmb_InputMesh meshB);
CMB_API cmb_Result* cmb_boolean_substract_mesh_cylinders(cmb_InputMesh mesh, uint32_t numCylinders, const cmb_CylinderInfo* cylinders);

CMB_API void cmb_release(cmb_Result* o);

CMB_API uint32_t cmb_numVertices(cmb_Result* o);
CMB_API uint32_t cmb_numTriangles(cmb_Result* o);
CMB_API float* cmb_positions(cmb_Result* o);
CMB_API float* cmb_normals(cmb_Result* o);
CMB_API uint32_t* cmb_indices(cmb_Result* o);
CMB_API void cmb_nothing(int x);