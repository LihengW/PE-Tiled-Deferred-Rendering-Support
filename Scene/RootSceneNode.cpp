#define NOMINMAX

#include "RootSceneNode.h"

#include "Light.h"
#include "DrawList.h"

#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "../Lua/LuaEnvironment.h"
#include "PrimeEngine/Render/ShaderActions/SetPerFrameConstantsShaderAction.h"
namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(RootSceneNode, SceneNode);

Handle RootSceneNode::s_hTitleInstance;
Handle RootSceneNode::s_hInstance;
Handle RootSceneNode::s_hCurInstance;

void RootSceneNode::Construct(PE::GameContext &context, PE::MemoryArena arena)
{
	Handle h("ROOT_SCENE_NODE", sizeof(RootSceneNode));
	RootSceneNode *pRootSceneNode = new(h) RootSceneNode(context, arena, h);
	pRootSceneNode->addDefaultComponents();
	SetInstance(h);
	
	s_hTitleInstance = Handle("ROOT_SCENE_NODE", sizeof(RootSceneNode));
	RootSceneNode *pTitleRootSceneNode = new(s_hTitleInstance) RootSceneNode(context, arena, h);
	pTitleRootSceneNode->addDefaultComponents();
	
	SetTitleAsCurrent();

	context.m_pRootSceneNode = pRootSceneNode;
}

void RootSceneNode::addDefaultComponents()
{
	SceneNode::addDefaultComponents();

	PE_REGISTER_EVENT_HANDLER(Events::Event_GATHER_DRAWCALLS, RootSceneNode::do_GATHER_DRAWCALLS);
	PE_REGISTER_EVENT_HANDLER(Events::Event_GATHER_DRAWCALLS_Z_ONLY, RootSceneNode::do_GATHER_DRAWCALLS);
}

void RootSceneNode::do_GATHER_DRAWCALLS(Events::Event *pEvt)
{
	DrawList *pDrawList = NULL;
	bool zOnly = pEvt->isInstanceOf<Events::Event_GATHER_DRAWCALLS_Z_ONLY>();
	
	pDrawList = zOnly ? DrawList::ZOnlyInstance() : DrawList::Instance();
	
	SceneNode *pRoot = RootSceneNode::Instance();
	Events::Event_GATHER_DRAWCALLS *pDrawEvent = zOnly ? NULL : (Events::Event_GATHER_DRAWCALLS *)(pEvt);
	Events::Event_GATHER_DRAWCALLS_Z_ONLY *pZOnlyDrawEvent = zOnly ? (Events::Event_GATHER_DRAWCALLS_Z_ONLY *)(pEvt) : NULL;

	// set some effect constants here that will be constant per frame

	unsigned char debug_bitmask = 0b00000001;

	debug_bitmask = debug_bitmask << m_debug_state;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			// should get the light masked
			m_TiledLightMask[3 * (8 * i + j) + 2] = debug_bitmask;
		}
	}

	bool setGlobalValues = true;
	if (!zOnly && pDrawEvent->m_drawOrder != EffectDrawOrder::First)
		setGlobalValues = false;

	if (setGlobalValues)
	{
		// fill in the data object that will be submitted to pipeline
		Handle &h = pDrawList->nextGlobalShaderValue();
		h = Handle("RAW_DATA", sizeof(SetPerFrameConstantsShaderAction));
		SetPerFrameConstantsShaderAction *p = new(h) SetPerFrameConstantsShaderAction(*m_pContext, m_arena);
		p->m_data.gGameTimes[0] = pDrawEvent ? pDrawEvent->m_gameTime : 0;
		p->m_data.gGameTimes[1] = pDrawEvent ? pDrawEvent->m_frameTime : 0;
	}

	// set some effect constants here that will be constant per object group
	// NOTE at this point we have only one object group so we set it on top level per frame

	if (setGlobalValues)
	{
		Handle &hsvPerObjectGroup = pDrawList->nextGlobalShaderValue();
		hsvPerObjectGroup = Handle("RAW_DATA", sizeof(SetPerObjectGroupConstantsShaderAction));
		SetPerObjectGroupConstantsShaderAction *psvPerObjectGroup = new(hsvPerObjectGroup) SetPerObjectGroupConstantsShaderAction(*m_pContext, m_arena);
	
		psvPerObjectGroup->m_data.gViewProj = pDrawEvent ? pDrawEvent->m_projectionViewTransform : pZOnlyDrawEvent->m_projectionViewTransform;
		psvPerObjectGroup->m_data.gViewInv = pDrawEvent ? pDrawEvent->m_viewInvTransform : Matrix4x4();
		// TODO: fill these in for motion blur
		psvPerObjectGroup->m_data.gPreviousViewProjMatrix = Matrix4x4();
		psvPerObjectGroup->m_data.gViewProjInverseMatrix = Matrix4x4();
		psvPerObjectGroup->m_data.gDoMotionBlur = 0;
		psvPerObjectGroup->m_data.gEyePosW = pDrawEvent ? pDrawEvent->m_eyePos : pZOnlyDrawEvent->m_eyePos;
		m_eyePos = psvPerObjectGroup->m_data.gEyePosW;

		// the light that drops shadows is defined by a boolean isShadowCaster in maya light objects
		PrimitiveTypes::UInt32 iDestLight = 0;
		if (pRoot->m_lights.m_size)
		{
			for(PrimitiveTypes::UInt32 i=0; i<(pRoot->m_lights.m_size); i++){
				Light *pLight = pRoot->m_lights[i].getObject<Light>();
				if(pLight->castsShadow()){
					Matrix4x4 worldToView = pLight->m_worldToViewTransform;
					Matrix4x4 lightProjectionViewWorldMatrix = (pLight->m_viewToProjectedTransform * worldToView);
					psvPerObjectGroup->m_data.gLightWVP = lightProjectionViewWorldMatrix;
					
					psvPerObjectGroup->m_data.gLights[iDestLight] = pLight->m_cbuffer;
					iDestLight++;

					break;
				}
			}
		}
		for (PrimitiveTypes::UInt32 iLight = 0;iLight < pRoot->m_lights.m_size; iLight++)
		{
			Light *pLight = pRoot->m_lights[iLight].getObject<Light>();
			if(pLight->castsShadow())
				continue;
			psvPerObjectGroup->m_data.gLights[iDestLight] = pLight->m_cbuffer;
			iDestLight++;
		}


		// tiled light detection

		if (pDrawEvent)
		{
			 // RGB for each pixel
			memset(m_TiledLightMask, 0, sizeof(m_TiledLightMask)); // clear all to black

			unsigned char bitmask = 0b00000001;

			for (int light_idx = 0; light_idx < pRoot->m_lights.m_size; light_idx++)
			{
				Light* pLight = pRoot->m_lights[light_idx].getObject<Light>();

				Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
				Vector3 foward = Vector3(0.0f, 0.0f, 1.0f);
				Vector3 right = Vector3(1.0f, 0.0f, 0.0f);


				if (pLight->m_cbuffer.type < 1.01f)
				{
					for (int i = 0; i < 8; i++)
					{
						for (int j = 0; j < 8; j++)
						{
								m_TiledLightMask[3 * (8 * i + j) + 0] |= bitmask;
						}
					}
				}
				else if (pLight->m_cbuffer.type < 2.01f && pLight->m_cbuffer.range > 0.01f)
				{
					Vector3& light_orign = pLight->m_cbuffer.pos;
					float light_range = pLight->m_cbuffer.range;
					light_range = 5.0f;

					Vector4 near_plane = pDrawEvent->m_frustum[0];
					Vector4 far_plane  = pDrawEvent->m_frustum[1];

					float near_p_distance = near_plane.m_x * light_orign.m_x +
						near_plane.m_y * light_orign.m_y +
						near_plane.m_z * light_orign.m_z +
						near_plane.m_w;

					float far_p_distance = far_plane.m_x * light_orign.m_x +
						far_plane.m_y * light_orign.m_y +
						far_plane.m_z * light_orign.m_z +
						far_plane.m_w;

					if (near_p_distance < -light_range || far_p_distance < -light_range)
					{
						// compeletly outside
						continue;
					}

					for (int i = 0; i < 8; i++)
					{
						for (int j = 0; j < 8; j++)
						{
							bool b_success = true;
							for (int k = 0; k < 4; k++)
							{
								Vector4& plane = pDrawEvent->m_subfrustum[4 * (8 * i + j) + k];
								float distance = plane.m_x * light_orign.m_x +
									plane.m_y * light_orign.m_y +
									plane.m_z * light_orign.m_z +
									plane.m_w;

								if (distance < -light_range)
								{
									b_success = false;
									break;
								}
							}

							if (b_success)
							{
								// should get the light masked
								m_TiledLightMask[3 * (8 * i + j) + 0] |= bitmask;
							}
						}
					}
				}

				bitmask = bitmask << 1;
			}
		}


		m_pglobalShaderAction = psvPerObjectGroup;
	}
}
}; // namespace Components
}; // namespace PE
