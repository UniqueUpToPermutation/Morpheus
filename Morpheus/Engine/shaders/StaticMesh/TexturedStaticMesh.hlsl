#ifndef TEXTURED_STATIC_MESH_
#define TEXTURED_STATIC_MESH_

#include "../Utils/BasicStructures.hlsl"

#ifdef __cplusplus
namespace HLSL {
#endif

struct StaticMeshInstanceData {
#ifdef __cplusplus
    float4x4 mWorld;
#else 
    matrix mWorldT;
#endif
};

#ifdef __cplusplus
}
#endif

#endif