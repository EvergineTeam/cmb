#ifdef _MSC_VER // Workaround for known bugs and issues on MSVC
	#define _HAS_STD_BYTE 0  // https://developercommunity.visualstudio.com/t/error-c2872-byte-ambiguous-symbol/93889
	#define NOMINMAX // https://stackoverflow.com/questions/1825904/error-c2589-on-stdnumeric-limitsdoublemin
#endif

#include "cmb.h"
#include "booleans.h"

typedef uint8_t u8;
typedef uint32_t u32;

struct ResultHeader {
	u32 numVertices;
	u32 numTriangles;
};

struct Vec3 { float x = 0, y = 0, z = 0; };

inline Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator-(Vec3 v) { return { -v.x, -v.y, -v.z }; }
inline Vec3 operator*(float a, Vec3 v) { return { a * v.x, a * v.y, a * v.z }; }
inline Vec3 operator/(Vec3 v, float d) { return { v.x / d, v.y / d, v.z / d }; }
inline void operator+=(Vec3& a, Vec3 b) { a = a + b; }
inline void operator-=(Vec3& a, Vec3 b) { a = a - b; }
inline void operator*=(Vec3& v, float f) { v = f * v; }
inline void operator/=(Vec3& v, float d) { v = v / d; }

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}
inline Vec3 normalized(Vec3 v) {
	float len2 = dot(v, v);
	return v / sqrtf(len2);
}

static void computeNormals(u32 numVertices, u32 numTriangles, const Vec3* positions, const u32* indices, Vec3* normals)
{
	std::fill(normals, normals + numVertices, Vec3{}); // initialize normals to {0,0,0}

	for (u32 triI = 0; triI < numTriangles; triI++) {
		const u32 i0 = indices[3 * triI + 0];
		const u32 i1 = indices[3 * triI + 1];
		const u32 i2 = indices[3 * triI + 2];
		const Vec3& v0 = positions[i0];
		const Vec3& v1 = positions[i1];
		const Vec3& v2 = positions[i1];
		const Vec3 n = cross(v1 - v0, v2 - v0);
		normals[i0] += n;
	}

	for (u32 i = 0; i < numVertices; i++)
		normals[i] = normalized(normals[i]);
}

CMB_API cmb_Result* cmb_boolean(cmb_BooleanType type,
	uint32_t numVerticesA, uint32_t numTrianglesA, float* positionsA, uint32_t* indicesA,
	uint32_t numVerticesB, uint32_t numTrianglesB, float* positionsB, uint32_t* indicesB)
{
	const auto op = (BoolOp)type;

	// prepare positions
	std::vector<double> inPositions;
	inPositions.reserve(3 * (numVerticesA + numVerticesB));
	for (size_t i = 0; i < 3 * numVerticesA; i++)
		inPositions.push_back(double(positionsA[i]));
	for (size_t i = 0; i < 3 * numVerticesB; i++)
		inPositions.push_back(double(positionsB[i]));
	
	// prepare indices
	std::vector<uint> inIndices;
	inIndices.reserve(3 * (numTrianglesA + numTrianglesB));
	for (size_t i = 0; i < 3 * numTrianglesA; i++)
		inIndices.push_back(uint(indicesA[i]));
	for (size_t i = 0; i < 3 * numTrianglesB; i++)
		inIndices.push_back(uint(numVerticesA + indicesB[i]));

	// the labels indicate, for each triangle, what object it belongs to
	std::vector<uint> inLabels(numTrianglesA + numTrianglesB);
	std::fill(inLabels.begin(), inLabels.begin() + numTrianglesA, 0);
	std::fill(inLabels.begin() + numTrianglesA, inLabels.end(), 1);

	// compute the boolean
	std::vector<double> outPositions;
	std::vector<uint> outIndices;
	std::vector<std::bitset<NBIT>> outLabels;
	booleanPipeline(
		inPositions, inIndices, inLabels,
		op,
		outPositions, outIndices, outLabels);

	// prepare result
	const u32 bufferSize_positions = sizeof(float) * outPositions.size();
	const u32 bufferSize_normals = bufferSize_positions;
	const u32 bufferSize_indices = sizeof(u32) * outIndices.size();

	u8* resultPtr = new u8[bufferSize_positions + bufferSize_normals + bufferSize_indices];

	const u32 resultOffset_positions = sizeof(ResultHeader);
	const u32 resultOffset_normals = resultOffset_positions + bufferSize_positions;
	const u32 resultOffset_indices = resultOffset_normals + bufferSize_normals;
	auto resultPtr_header = (ResultHeader*)resultPtr;
	auto resultPtr_positions = (float*)(resultPtr + resultOffset_positions);
	auto resultPtr_normals = (float*)(resultPtr + resultOffset_normals);
	auto resultPtr_indices = (u32*)(resultPtr + resultOffset_indices);

	const u32 numVertices = outPositions.size() / 3;
	const u32 numTriangles = outIndices.size() / 3;
	*resultPtr_header = { numVertices, numTriangles };

	for (u32 i = 0; i < outPositions.size(); i++)
		resultPtr_positions[i] = float(outPositions[i]);

	for (u32 i = 0; i < outIndices.size(); i++)
		resultPtr_indices[i] = u32(outIndices[i]);

	computeNormals(numVertices, numTriangles, (Vec3*)resultPtr_positions, resultPtr_indices, (Vec3*)resultPtr_normals);

	return (cmb_Result*)resultPtr;
}

CMB_API void cmb_release(cmb_Result* o)
{
	auto ptr = (u8*)o;
	delete[] ptr;
}

CMB_API uint32_t cmb_numVertices(cmb_Result* o)
{
	auto header = (ResultHeader*)o;
	return header->numVertices;
}
CMB_API uint32_t cmb_numTriangles(cmb_Result* o)
{
	auto header = (ResultHeader*)o;
	return header->numTriangles;
}
CMB_API float* cmb_positions(cmb_Result* o)
{
	auto header = (ResultHeader*)o;
	auto ptr = (u8*)o;
	return (float*)(ptr + sizeof(ResultHeader));
}
CMB_API float* cmb_normals(cmb_Result* o)
{
	auto header = (ResultHeader*)o;
	auto ptr = (u8*)o;
	return (float*)(ptr + sizeof(ResultHeader) + 3 * sizeof(float) * header->numVertices);
}
CMB_API uint32_t* cmb_indices(cmb_Result* o)
{
	auto header = (ResultHeader*)o;
	auto ptr = (u8*)o;
	return (u32*)(ptr + sizeof(ResultHeader) + 2 * 3 * sizeof(float) * header->numVertices);
}