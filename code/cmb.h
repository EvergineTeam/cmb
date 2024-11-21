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

struct cmb_Result;

CMB_API cmb_Result* cmb_boolean(cmb_BooleanType type,
	uint32_t numVerticesA, uint32_t numTrianglesA, float* positionsA, uint32_t* indicesA,
	uint32_t numVerticesB, uint32_t numTrianglesB, float* positionsB, uint32_t* indicesB);

CMB_API void cmb_release(cmb_Result* o);

CMB_API uint32_t cmb_numVertices(cmb_Result* o);
CMB_API uint32_t cmb_numTriangles(cmb_Result* o);
CMB_API float* cmb_positions(cmb_Result* o);
CMB_API float* cmb_normals(cmb_Result* o);
CMB_API uint32_t* cmb_indices(cmb_Result* o);