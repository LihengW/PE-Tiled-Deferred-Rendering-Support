#ifndef DEFERRED_SHADING_STRUCTS
#define DEFERRED_SHADING_STRUCTS

#include "GBufferWrite_Structs.fx"
#include "APIAbstraction.gpu"

// ----------------------------------------------------------------
// Vertex Input Struct
// ----------------------------------------------------------------

struct DEFERRED_SHADING_VS_IN
{
    float3 iPosL       API_SEMANTIC(VSIN_POSITION);
    float2 iTexCoord   API_SEMANTIC(TEXCOORD0);
    float3 iNormal     API_SEMANTIC(VSIN_NORMAL);
    float3 iColor      API_SEMANTIC(VSIN_COLOR);
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

struct DEFERRED_SHADING_PS_IN
{
    float4 iPosH       API_SEMANTIC(PIPELINE_POSITION);
    float3 iPosW       API_SEMANTIC(PSIN_EXTRA_POSITION);
    float2 iTexCoord   API_SEMANTIC(TEXCOORD0);
    float3 iNormalW    API_SEMANTIC(PIPELINE_NORMAL);
    float3 iColor      API_SEMANTIC(PSIN_COLOR);
};

// ----------------------------------------------------------------
// G-buffer Output Struct
// ----------------------------------------------------------------


#define VS_wrapper_DEFERRED_SHADING(func) \
        DEFERRED_SHADING_PS_IN main(DEFERRED_SHADING_VS_IN API_NOSTRIP vIn) { \
            DEFERRED_SHADING_PS_IN pIn; \
            pIn = func(vIn); \
            return pIn; \
        }

#define PS_wrapper_DEFERRED_SHADING(func) \
        float4 main(DEFERRED_SHADING_PS_IN pIn) : PSOUT_COLOR \
        { \
            return func(pIn); \
        }


#endif // HLSL_DEFERRED_GBUFFER_STRUCTS
