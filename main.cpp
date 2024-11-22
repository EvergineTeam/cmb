#define STB_IMA_IMPLEMENTATIONGE_DISABLE
#include <cmb.h>

#ifdef _MSC_VER // Workaround for known bugs and issues on MSVC
    #define _HAS_STD_BYTE 0  // https://developercommunity.visualstudio.com/t/error-c2872-byte-ambiguous-symbol/93889
    #define NOMINMAX // https://stackoverflow.com/questions/1825904/error-c2589-on-stdnumeric-limitsdoublemin
#endif
#include "io_functions.h"

struct Mesh {
    std::vector<float> positions;
    std::vector<uint32_t> indices;
};

Mesh loadMesh(const std::string path)
{
    std::vector<double> positions;
    std::vector<uint> indices;
    std::vector<uint> labels;

    loadMultipleFiles({ path }, positions, indices, labels);

    return Mesh{
        std::vector<float>{positions.begin(), positions.end()},
        std::vector<uint32_t>{indices}
    };
}

int main(int argc, char **argv)
{
    cmb_BooleanType op;
    if(argc != 5)
    {
        std::cout << "syntax error!" << std::endl;
        std::cout << "./exact_boolean BOOL_OPERATION (intersection OR union OR subtraction) input1.obj input2.obj output.obj" << std::endl;
        return -1;
    }
    else
    {
        if (strcmp(argv[1], "intersection") == 0)       op = CMB_INTERSECTION;
        else if (strcmp(argv[1], "union") == 0)         op = CMB_UNION;
        else if (strcmp(argv[1], "subtraction") == 0)   op = CMB_DIFFERENCE;
        else if (strcmp(argv[1], "xor") == 0)           op = CMB_XOR;
        else {
            printf("Invalid boolean type %s\n", argv[1]);
            exit(1);
        }
    }

    std::vector<std::string> files;
    for(int i = 2; i < (argc -1); i++)
        files.emplace_back(argv[i]);
    Mesh meshA = loadMesh(argv[2]);
    Mesh meshB = loadMesh(argv[3]);

    std::string file_out = argv[4];

    auto result = cmb_boolean(op,
        { uint32_t(meshA.positions.size() / 3), uint32_t(meshA.indices.size() / 3), meshA.positions.data(), meshA.indices.data() },
        { uint32_t(meshB.positions.size() / 3), uint32_t(meshB.indices.size() / 3), meshB.positions.data(), meshB.indices.data() }
    );

    auto positionsPtr = cmb_positions(result);
    std::vector<double> resultPositons(positionsPtr, positionsPtr + 3 * cmb_numVertices(result));

    auto indicesPtr = cmb_indices(result);
    std::vector<uint> resultIndices(indicesPtr, indicesPtr + 3 * cmb_numTriangles(result));

    cinolib::write_OBJ(file_out.c_str(), resultPositons, resultIndices, {});

    return 0;
}