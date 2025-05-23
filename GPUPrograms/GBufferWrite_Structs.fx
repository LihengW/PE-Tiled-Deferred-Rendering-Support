// #ifndef DEFERRED_GBUFFER_STRUCTS
// #define DEFERRED_GBUFFER_STRUCTS
// 
// #include "StdMesh_Structs.fx"
// #include "DetailedMesh_Structs.fx"
// #include "APIAbstraction.gpu" // For API_SEMANTIC and platform abstraction
// 
// // -------------------------------------------------------------
// // Vertex Shader Input Struct
// // -------------------------------------------------------------
// struct DEFERRED_MESH_VS_IN
// {
//     float3 iPosL        API_SEMANTIC(VSIN_POSITION);
//     float2 iTexCoord    API_SEMANTIC(TEXCOORD0);
//     float3 iNormal      API_SEMANTIC(VSIN_NORMAL);
//     float3 iTangent     API_SEMANTIC(VSIN_TANGENT);
//     float3 iColor       API_SEMANTIC(VSIN_COLOR);
// };
// 
// // -------------------------------------------------------------
// // Vertex Shader Output / Pixel Shader Input Struct
// // -------------------------------------------------------------
// struct DEFERRED_MESH_PS_IN
// {
//     float4 iPosH     API_SEMANTIC(PIPELINE_POSITION);
//     float3 iPosW     API_SEMANTIC(PSIN_EXTRA_POSITION);
//     float2 iTexCoord API_SEMANTIC(TEXCOORD0);
//     float3 iNormalW  API_SEMANTIC(PIPELINE_NORMAL);
//     float3 iTangentW API_SEMANTIC(PSIN_TANGENT);
//     float3 iColor       API_SEMANTIC(PSIN_COLOR);
// };
// 
// // -------------------------------------------------------------
// // Pixel Shader Output Struct (G-buffer)
// // -------------------------------------------------------------
// struct DEFERRED_GBUFFER_OUTPUT
// {
//     float4 gAlbedo : COLOR0;
//     float4 gNormal : COLOR1;
//     float4 gPosition : COLOR2;
// };
// 
// 
// #define VS_wrapper_DEFERRED_MESH(func) \
//         DEFERRED_MESH_PS_IN main(DEFERRED_MESH_VS_IN API_NOSTRIP vIn) { \
//             DEFERRED_MESH_PS_IN pIn; \
//             pIn = func(vIn); \
//             return pIn; \
//         }
// 
// #define PS_wrapper_DEFERRED_MESH(func) \
//         DEFERRED_GBUFFER_OUTPUT main(DEFERRED_MESH_PS_IN pIn) : DEFERRED_GBUFFER_OUTPUT\
//         { \
//             return func(pIn); \
//         }
// 
// 
// 
// #define PS_wrapper_DEFERRED_MESH_POS(func) \
//         float4 main(DEFERRED_MESH_PS_IN pIn) : PSOUT_COLOR \
//         { \
//             return func(pIn); \
//         }
// 
// #define PS_wrapper_DEFERRED_MESH_NORMAL(func) \
//         float4 main(DEFERRED_MESH_PS_IN pIn) : COLOR1 \
//         { \
//             return func(pIn); \
//         }
// 
// #define PS_wrapper_DEFERRED_MESH_ALBEDO(func) \
//         float4 main(DEFERRED_MESH_PS_IN pIn) : COLOR2 \
//         { \
//             return func(pIn); \
//         }
// 
// #define PS_wrapper_DEFERRED_MESH_SPEC(func) \
//         float4 main(DEFERRED_MESH_PS_IN pIn) : COLOR3 \
//         { \
//             return func(pIn); \
//         }
// 
// #endif 

#ifndef DEFERRED_GBUFFER_STRUCTS
#define DEFERRED_GBUFFER_STRUCTS

#include "StdMesh_Structs.fx"
#include "APIAbstraction.gpu"

// ----------------------------------------------------------------
// Vertex Input Struct
// ----------------------------------------------------------------

struct DEFERRED_MESH_VS_IN
{
    float3 iPosL       API_SEMANTIC(VSIN_POSITION);
    float2 iTexCoord   API_SEMANTIC(TEXCOORD0);
    float3 iNormal     API_SEMANTIC(VSIN_NORMAL);
    float3 iTangent    API_SEMANTIC(VSIN_TANGENT);
    float3 iColor      API_SEMANTIC(VSIN_COLOR);
};

struct DEFERRED_SKIN_VS_IN
{
    float3 iPosL        API_SEMANTIC(VSIN_POSITION);
    float4 jointWeights  API_SEMANTIC(VSIN_JOINTWEIGHTS0);
#if DEFAULT_SKIN_WEIGHTS_PER_VERTEX == 8
    float4 jointWeights1 API_SEMANTIC(VSIN_JOINTWEIGHTS1);
#endif

    float4 jointIndices  API_SEMANTIC(VSIN_BONEINDICES0);
#if DEFAULT_SKIN_WEIGHTS_PER_VERTEX == 8
    float4 jointIndices1 API_SEMANTIC(VSIN_BONEINDICES1);
#endif

    float2 iTexCoord    API_SEMANTIC(TEXCOORD0);
    float3 iNormal      API_SEMANTIC(VSIN_NORMAL);
    float3 iTangent     API_SEMANTIC(VSIN_TANGENT);
};


#if APIABSTRACTION_D3D11
struct DEFERRED_MESH_INSTANCED_VS_IN
{
    float3 iPosL       API_SEMANTIC(VSIN_POSITION);
    float2 iTexCoord   API_SEMANTIC(TEXCOORD0);
    float3 iNormal     API_SEMANTIC(VSIN_NORMAL);
    float3 iTangent    API_SEMANTIC(VSIN_TANGENT);
    float3 iColor      API_SEMANTIC(VSIN_COLOR);
    uint   iInstanceId API_SEMANTIC(SV_InstanceID);
};
#endif

// ----------------------------------------------------------------
// Vertex Output / Pixel Input Struct
// ----------------------------------------------------------------

struct DEFERRED_MESH_PS_IN
{
    float4 iPosH                API_SEMANTIC(PIPELINE_POSITION);
    float3 iPosW                API_SEMANTIC(PSIN_EXTRA_POSITION);
    float2 iTexCoord            API_SEMANTIC(TEXCOORD0);
    float3 iNormalW             API_SEMANTIC(PIPELINE_NORMAL);
    float3 iTangentW            API_SEMANTIC(PSIN_TANGENT);
    float3 iColor               API_SEMANTIC(PSIN_COLOR);
    float4 iProjTexCoord        API_SEMANTIC(PSIN_PROJECTION);
};

// ----------------------------------------------------------------
// G-buffer Output Struct
// ----------------------------------------------------------------

struct DEFERRED_GBUFFER_OUTPUT
{
    float4 gAlbedo : COLOR0;
    float4 gNormal : COLOR1;
    float4 gPosition : COLOR2;
};


#define VS_wrapper_DEFERRED_MESH(func) \
        DEFERRED_MESH_PS_IN main(DEFERRED_MESH_VS_IN API_NOSTRIP vIn) { \
            DEFERRED_MESH_PS_IN pIn; \
            pIn = func(vIn); \
            return pIn; \
        }

#define PS_wrapper_DEFERRED_MESH(func) \
        DEFERRED_GBUFFER_OUTPUT main(DEFERRED_MESH_PS_IN pIn) : DEFERRED_GBUFFER_OUTPUT \
        { \
            return func(pIn); \
        }

#define VS_wrapper_DEFERRED_SKIN(func) \
        DEFERRED_MESH_PS_IN main(DEFERRED_SKIN_VS_IN API_NOSTRIP vIn) { \
        DEFERRED_MESH_PS_IN pIn; \
        pIn = func(vIn); \
        return pIn; \
    }

	
#define VS_wrapper_DEFERRED_SKIN_SHADOWED(func) \
        DEFERRED_MESH_PS_IN main(DEFERRED_SKIN_VS_IN API_NOSTRIP vIn) { \
        DEFERRED_MESH_PS_IN pIn; \
        pIn = func(vIn); \
        return pIn; \
    }

// NOT USED
#define VS_wrapper_DEFERRED_SKIN_INSTANCED(func) \
        DEFERRED_MESH_PS_IN main(DEFERRED_SKIN_INSTANCED_VS_IN vIn) { \
        DEFERRED_MESH_PS_IN pIn; \
        pIn = func(vIn); \
        return pIn; \
    }
#define VS_wrapper_DEFERRED_SKIN_SHADOWED_INSTANCED(func) \
        DEFERRED_MESH_PS_IN main(DEFERRED_SKIN_INSTANCED_VS_IN vIn) { \
        DEFERRED_MESH_PS_IN pIn; \
        pIn = func(vIn); \
        return pIn; \
    }


#endif // HLSL_DEFERRED_GBUFFER_STRUCTS
