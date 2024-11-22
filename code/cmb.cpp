#ifdef _MSC_VER // Workaround for known bugs and issues on MSVC
	#define _HAS_STD_BYTE 0  // https://developercommunity.visualstudio.com/t/error-c2872-byte-ambiguous-symbol/93889
	#define NOMINMAX // https://stackoverflow.com/questions/1825904/error-c2589-on-stdnumeric-limitsdoublemin
#endif

#include "cmb.h"
#include "booleans.h"
#include <span>

typedef uint8_t u8;
typedef uint32_t u32;
template <typename T> using CSpan = std::span<const T>;

constexpr float PI = 3.14159265f;
constexpr u32 cylinderResolution = 32;
constexpr u32 cylinderNumVerts = 2 * cylinderResolution;
constexpr u32 cylinderNumTris = (cylinderResolution - 1) * 4;

struct ResultHeader {
	u32 numVertices;
	u32 numTriangles;
};

// --- Vec3 --------------------
template<typename T>
struct Vec3 {
	T x = 0, y = 0, z = 0;

	template <typename T2>
	operator Vec3<T2>() { return { T1(x), T1(y), T1(z) }; }
};



template<typename T> Vec3<T> operator+(Vec3<T> a, Vec3<T> b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
template<typename T> Vec3<T> operator-(Vec3<T> a, Vec3<T> b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
template<typename T> Vec3<T> operator-(Vec3<T> v) { return { -v.x, -v.y, -v.z }; }
template<typename T> Vec3<T> operator*(T a, Vec3<T> v) { return { a * v.x, a * v.y, a * v.z }; }
template<typename T> Vec3<T> operator/(Vec3<T> v, T d) { return { v.x / d, v.y / d, v.z / d }; }
template<typename T> void operator+=(Vec3<T>& a, Vec3<T> b) { a = a + b; }
template<typename T> void operator-=(Vec3<T>& a, Vec3<T> b) { a = a - b; }
template<typename T> void operator*=(Vec3<T>& v, T f) { v = f * v; }
template<typename T> void operator/=(Vec3<T>& v, T d) { v = v / d; }

template<typename T>
float dot(Vec3<T> a, Vec3<T> b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

template<typename T>
Vec3<T> cross(Vec3<T> a, Vec3<T> b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

template<typename T>
Vec3<T> normalized(Vec3<T> v) {
	T len2 = dot(v, v);
	return v / std::sqrt(len2);
}

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
// ----------------------------------

static void computeNormals(u32 numVertices, u32 numTriangles, const Vec3f* positions, const u32* indices, Vec3f* normals)
{
	std::fill(normals, normals + numVertices, Vec3f{}); // initialize normals to {0,0,0}

	for (u32 triI = 0; triI < numTriangles; triI++) {
		const u32 i0 = indices[3 * triI + 0];
		const u32 i1 = indices[3 * triI + 1];
		const u32 i2 = indices[3 * triI + 2];
		const Vec3f& v0 = positions[i0];
		const Vec3f& v1 = positions[i1];
		const Vec3f& v2 = positions[i1];
		const Vec3f n = cross(v1 - v0, v2 - v0);
		normals[i0] += n;
	}

	for (u32 i = 0; i < numVertices; i++)
		normals[i] = normalized(normals[i]);
}

static void makeCylinder(
	std::vector<double>& positions, std::vector<uint>& triangles,
	Vec3d center,
	Vec3d basisX, Vec3d basisY, Vec3d basisZ,
	float radius, float halfHeight, u32 resolution)
{
	const u32 numVerts = cylinderNumVerts;
	positions.resize(3 * numVerts);
	const float deltaAlpha = 2 * PI / resolution;
	for (u32 i = 0; i < resolution; i++) {
		const float alpha = i * deltaAlpha;
		float z = radius * cos(alpha);
		float x = radius * sin(alpha);

		positions[3*i + 0] = x;
		positions[3*i + 1] = -halfHeight;
		positions[3*i + 2] = z;
	}

	for (u32 i = 0; i < resolution; i++) {
		positions[3*(resolution + i) + 0] = positions[3*i + 0];
		positions[3*(resolution + i) + 1] = -positions[3*i + 1];
		positions[3*(resolution + i) + 2] = positions[3*i + 2];
	}

	for (u32 i = 0; i < numVerts; i++) {
		auto& v = *(Vec3d*)&positions[3 * i];
		v = center + v.x * basisX + v.y * basisY + v.z * basisZ;
	}

	triangles.resize(3 * cylinderNumTris);
	uint triI = 0;
	for (uint i = 0; i < resolution - 2; i++) {
		triangles[3*triI + 0] = i + 1;
		triangles[3*triI + 1] = 0;
		triangles[3*triI + 2] = i + 2;
		triI++;
	}
	for (uint i = 0; i < resolution - 2; i++) {
		triangles[3*triI + 0] = resolution;
		triangles[3*triI + 1] = resolution + i + 1;
		triangles[3*triI + 2] = resolution + i + 2;
		triI++;
	}
	for (uint i = 0; i < resolution; i++) {
		const uint i00 = i;
		const uint i01 = (i + 1) % resolution;
		const uint i10 = i + resolution;
		const uint i11 = i01 + resolution;
		triangles[3*triI + 0] = i11;
		triangles[3*triI + 1] = i10;
		triangles[3*triI + 2] = i00;
		triI++;
		triangles[3*triI + 0] = i11;
		triangles[3*triI + 1] = i00;
		triangles[3*triI + 2] = i01;
		triI++;
	}
}

// A = A <op> B
static void calcBooleanOp(BoolOp op,
	std::vector<double>& positionsA, std::vector<uint>& indicesA,
	CSpan<double> positionsB, CSpan<uint> indicesB)
{
	const uint numTrisA = indicesA.size() / 3;
	const uint numTrisB = indicesB.size() / 3;
	const uint firstIndB = positionsA.size() / 3;

	// incorporate B positions to A
	positionsA.reserve(positionsA.size() + positionsB.size());
	for (auto p : positionsB)
		positionsA.push_back(p);

	// incorporate B indices to A
	indicesA.reserve(indicesA.size() + indicesB.size());
	for (auto ind : indicesB)
		indicesA.push_back(firstIndB + ind);

	// the labels indicate, for each triangle, what object it belongs to
	std::vector<uint> labels;
	labels.reserve(numTrisA + numTrisB);
	for (uint i = 0; i < numTrisA; i++)
		labels.push_back(0);
	for (uint i = 0; i < numTrisB; i++)
		labels.push_back(1);

	static std::vector<double> positionsOut;
	static std::vector<uint> indicesOut;
	static std::vector<std::bitset<NBIT>> outLabels;
	positionsOut.clear();
	indicesOut.clear();
	outLabels.clear();
	outLabels.reserve(numTrisA + numTrisB);

	booleanPipeline(
		positionsA, indicesA, labels,
		op,
		positionsOut, indicesOut, outLabels);

	std::swap(positionsA, positionsOut);
	std::swap(indicesA, indicesOut);
}

static cmb_Result* prepareResult(CSpan<double> positions, CSpan<uint> indices)
{
	// prepare result
	const u32 bufferSize_positions = sizeof(float) * positions.size();
	const u32 bufferSize_normals = bufferSize_positions;
	const u32 bufferSize_indices = sizeof(u32) * indices.size();

	u8* resultPtr = new u8[bufferSize_positions + bufferSize_normals + bufferSize_indices];

	const u32 resultOffset_positions = sizeof(ResultHeader);
	const u32 resultOffset_normals = resultOffset_positions + bufferSize_positions;
	const u32 resultOffset_indices = resultOffset_normals + bufferSize_normals;
	auto resultPtr_header = (ResultHeader*)resultPtr;
	auto resultPtr_positions = (float*)(resultPtr + resultOffset_positions);
	auto resultPtr_normals = (float*)(resultPtr + resultOffset_normals);
	auto resultPtr_indices = (u32*)(resultPtr + resultOffset_indices);

	const u32 numVertices = positions.size() / 3;
	const u32 numTriangles = indices.size() / 3;
	*resultPtr_header = { numVertices, numTriangles };

	for (u32 i = 0; i < positions.size(); i++)
		resultPtr_positions[i] = float(positions[i]);

	for (u32 i = 0; i < indices.size(); i++)
		resultPtr_indices[i] = u32(indices[i]);

	computeNormals(numVertices, numTriangles, (Vec3f*)resultPtr_positions, resultPtr_indices, (Vec3f*)resultPtr_normals);

	return (cmb_Result*)resultPtr;
}

CMB_API cmb_Result* cmb_boolean(cmb_BooleanType type, cmb_InputMesh meshA, cmb_InputMesh meshB)
{
	const auto op = (BoolOp)type;
	const uint32_t numVerticesA = meshA.numVertices;
	const uint32_t numTrianglesA = meshA.numTriangles;
	const uint32_t numVerticesB = meshB.numVertices;
	const uint32_t numTrianglesB = meshB.numTriangles;

	// prepare positions
	std::vector<double> positionsA;
	positionsA.reserve(3 * (numVerticesA + numVerticesB));
	for (size_t i = 0; i < 3 * numVerticesA; i++)
		positionsA.push_back(double(meshA.positions[i]));

	std::vector<double> positionsB;
	positionsB.reserve(3 * numVerticesB);
	for (size_t i = 0; i < 3 * numVerticesB; i++)
		positionsB.push_back(double(meshB.positions[i]));
	
	// prepare indices
	std::vector<uint> indicesA;
	indicesA.reserve(3 * (numTrianglesA + numTrianglesB));
	for (size_t i = 0; i < 3 * numTrianglesA; i++)
		indicesA.push_back(uint(meshA.indices[i]));

	std::vector<uint> indicesB;
	indicesB.reserve(3 * numTrianglesB);
	for (size_t i = 0; i < 3 * numTrianglesB; i++)
		indicesB.push_back(uint(meshB.indices[i]));

	calcBooleanOp(op, positionsA, indicesA, positionsB, indicesB);

	return prepareResult(positionsA, indicesA);
}

CMB_API cmb_Result* cmb_boolean_substract_mesh_cylinders(cmb_InputMesh mesh, uint32_t numCylinders, const cmb_CylinderInfo* cylinders)
{
	std::vector<double> meshPositions;
	meshPositions.reserve(3 * (mesh.numVertices + cylinderNumVerts));
	for (u32 i = 0; i < 3 * mesh.numVertices; i++)
		meshPositions.push_back(double(mesh.positions[i]));

	std::vector<uint> meshIndices;
	meshIndices.reserve(3 * (mesh.numTriangles + cylinderNumTris));
	for (u32 i = 0; i < 3 * mesh.numTriangles; i++)
		meshIndices.push_back(double(mesh.indices[i]));

	std::vector<double> cylinderPositions;
	std::vector<uint> cylinderIndices;
	for (u32 cylI = 0; cylI < numCylinders; cylI++) {
		auto& cylinder = cylinders[cylI];

		const Vec3d basisY(cylinder.dirX, cylinder.dirY, cylinder.dirZ);
		const Vec3d basisX = normalized(cross(basisY, abs(basisY.z) > 0.1 ? Vec3d(0, 0, 1) : Vec3d(1, 0, 0)));
		const Vec3d basisZ = cross(basisX, basisY);

		makeCylinder(cylinderPositions, cylinderIndices,
			Vec3d{ cylinder.posX, cylinder.posY, cylinder.posZ },
			basisX, basisY, basisZ,
			cylinder.radius, cylinder.halfHeight, cylinderResolution
		);

		calcBooleanOp(BoolOp::SUBTRACTION, meshPositions, meshIndices, cylinderPositions, cylinderIndices);
	}

	return prepareResult(meshPositions, meshIndices);
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