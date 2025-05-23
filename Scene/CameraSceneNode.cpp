#include "CameraSceneNode.h"
#include "../Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"

#define Z_ONLY_CAM_BIAS 0.0f
namespace PE {
namespace Components {

PE_IMPLEMENT_CLASS1(CameraSceneNode, SceneNode);

CameraSceneNode::CameraSceneNode(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself) : SceneNode(context, arena, hMyself)
{
	m_near = 0.05f;
	m_far = 2000.0f;
}
void CameraSceneNode::addDefaultComponents()
{
	Component::addDefaultComponents();
	PE_REGISTER_EVENT_HANDLER(Events::Event_CALCULATE_TRANSFORMATIONS, CameraSceneNode::do_CALCULATE_TRANSFORMATIONS);
}

void CameraSceneNode::do_CALCULATE_TRANSFORMATIONS(Events::Event *pEvt)
{
	Handle hParentSN = getFirstParentByType<SceneNode>();
	if (hParentSN.isValid())
	{
		Matrix4x4 parentTransform = hParentSN.getObject<PE::Components::SceneNode>()->m_worldTransform;
		m_worldTransform = parentTransform * m_base;
	}
	
	Matrix4x4 &mref_worldTransform = m_worldTransform;

	Vector3 pos = Vector3(mref_worldTransform.m[0][3], mref_worldTransform.m[1][3], mref_worldTransform.m[2][3]);
	Vector3 n = Vector3(mref_worldTransform.m[0][2], mref_worldTransform.m[1][2], mref_worldTransform.m[2][2]);
	Vector3 target = pos + n;
	Vector3 up = Vector3(mref_worldTransform.m[0][1], mref_worldTransform.m[1][1], mref_worldTransform.m[2][1]);

	m_worldToViewTransform = CameraOps::CreateViewMatrix(pos, target, up);

	m_worldTransform2 = mref_worldTransform;

	m_worldTransform2.moveForward(Z_ONLY_CAM_BIAS);

	Vector3 pos2 = Vector3(m_worldTransform2.m[0][3], m_worldTransform2.m[1][3], m_worldTransform2.m[2][3]);
	Vector3 n2 = Vector3(m_worldTransform2.m[0][2], m_worldTransform2.m[1][2], m_worldTransform2.m[2][2]);
	Vector3 target2 = pos2 + n2;
	Vector3 up2 = Vector3(m_worldTransform2.m[0][1], m_worldTransform2.m[1][1], m_worldTransform2.m[2][1]);

	m_worldToViewTransform2 = CameraOps::CreateViewMatrix(pos2, target2, up2);
    
    PrimitiveTypes::Float32 aspect = (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getWidth()) / (PrimitiveTypes::Float32)(m_pContext->getGPUScreen()->getHeight());
    
    // PrimitiveTypes::Float32 verticalFov = 0.33f * PrimitiveTypes::Constants::c_Pi_F32;
	PrimitiveTypes::Float32 verticalFov = m_verticle_fov;

    if (aspect < 1.0f)
    {
        //ios portrait view
        static PrimitiveTypes::Float32 factor = 0.5f;
        verticalFov *= factor;
    }

	// float detection_offseted_fov = m_verticle_fov - 0.1f * PrimitiveTypes::Constants::c_Pi_F32;
	float detection_offseted_fov = m_verticle_fov;
	// calculation of frustum
	n.normalize();
	Vector3 near_p = pos + m_near * n;
	m_frustum[0] = Vector4(n.getX(), n.getY(), n.getZ(), -(near_p.m_x * n.getX() + near_p.m_y * n.getY() + near_p.m_z * n.getZ()));
	Vector3 far_p = pos + m_far * n;
	m_frustum[1] = Vector4(-n.getX(), -n.getY(), -n.getZ(), far_p.m_x * n.getX() + far_p.m_y * n.getY() + far_p.m_z * n.getZ());

	float h_near = 2.0f * m_near * tanf(detection_offseted_fov / 2.0f);
	float w_near = aspect * h_near;
	float h_far = 2.0f * m_far * tanf(detection_offseted_fov / 2.0f);
	float w_far = aspect * h_far;

	Vector3 right = n.crossProduct(up);
	right.normalize();

	Vector3 near_top_left = near_p + h_near / 2.0f * up - w_near / 2.0f * right;
	Vector3 near_top_right = near_p + h_near / 2.0f * up + w_near / 2.0f * right;
	Vector3 near_down_left = near_p - h_near / 2.0f * up - w_near / 2.0f * right;
	Vector3 near_down_right = near_p - h_near / 2.0f * up + w_near / 2.0f * right;


	Vector3 far_top_left = far_p + h_far / 2.0f * up - w_far / 2.0f * right;
	Vector3 far_top_right = far_p + h_far / 2.0f * up + w_far / 2.0f * right;
	Vector3 far_down_left = far_p - h_far / 2.0f * up - w_far / 2.0f * right;
	Vector3 far_down_right = far_p - h_far / 2.0f * up + w_far / 2.0f * right;

	Vector3 left_plane_n = (far_down_left - near_down_left).crossProduct(near_top_left - near_down_left);
	left_plane_n.normalize();

	Vector4 left_plane = Vector4(left_plane_n.getX(), left_plane_n.getY(), left_plane_n.getZ(), -(near_down_left.m_x * left_plane_n.getX() + near_down_left.m_y * left_plane_n.getY() + near_down_left.m_z * left_plane_n.getZ()));

	Vector3 right_plane_n = (near_top_right - near_down_right).crossProduct(far_down_right - near_down_right);
	right_plane_n.normalize();

	Vector4 right_plane = Vector4(right_plane_n.getX(), right_plane_n.getY(), right_plane_n.getZ(), -(near_down_right.m_x * right_plane_n.getX() + near_down_right.m_y * right_plane_n.getY() + near_down_right.m_z * right_plane_n.getZ()));

	Vector3 up_plane_n = (far_top_left - near_top_left).crossProduct(near_top_right - near_top_left);
	up_plane_n.normalize();

	Vector4 up_plane = Vector4(up_plane_n.getX(), up_plane_n.getY(), up_plane_n.getZ(), -(near_top_left.m_x * up_plane_n.getX() + near_top_left.m_y * up_plane_n.getY() + near_top_left.m_z * up_plane_n.getZ()));

	Vector3 down_plane_n = (near_down_right - near_down_left).crossProduct(far_down_left - near_down_left);
	down_plane_n.normalize();

	Vector4 down_plane = Vector4(down_plane_n.getX(), down_plane_n.getY(), down_plane_n.getZ(), -(near_down_left.m_x * down_plane_n.getX() + near_down_left.m_y * down_plane_n.getY() + near_down_left.m_z * down_plane_n.getZ()));

	m_frustum[2] = left_plane;
	m_frustum[3] = right_plane;
	m_frustum[4] = up_plane;
	m_frustum[5] = down_plane;

	m_viewToProjectedTransform = CameraOps::CreateProjectionMatrix(verticalFov, 
		aspect,
		m_near, m_far);


	Vector3 sub_near_origin = near_down_left;
	Vector3 sub_far_origin = far_down_left;

	float h_near_tiled = h_near / 8.0f;
	float w_near_tiled = w_near / 8.0f;
	float h_far_tiled = h_far / 8.0f;
	float w_far_tiled = w_far / 8.0f;

	for (int tile_y_idx = 0; tile_y_idx < 8; tile_y_idx++) {
		for (int tile_x_idx = 0; tile_x_idx < 8; tile_x_idx++) {

			int tileIndex = tile_y_idx * 8 + ( 7 - tile_x_idx );
			// Vector3 sub_near_top_left = sub_near_origin + h_near / 2.0f * up - w_near / 2.0f * right;
			// Vector3 sub_near_top_right = sub_near_origin + h_near / 2.0f * up + w_near / 2.0f * right;
			Vector3 sub_near_top_left = sub_near_origin + h_near_tiled * (tile_y_idx + 1) * up + w_near_tiled * tile_x_idx * right;
			Vector3 sub_near_top_right = sub_near_origin + h_near_tiled * (tile_y_idx + 1) * up + w_near_tiled * (tile_x_idx + 1) * right;
			Vector3 sub_near_down_left = sub_near_origin + h_near_tiled * tile_y_idx * up + w_near_tiled * tile_x_idx * right;
			Vector3 sub_near_down_right = sub_near_origin + h_near_tiled * tile_y_idx * up + w_near_tiled * (tile_x_idx + 1) * right;

			Vector3 sub_far_top_left   = sub_far_origin + h_far_tiled * (tile_y_idx + 1) * up + w_far_tiled * tile_x_idx * right;
			Vector3 sub_far_top_right  = sub_far_origin + h_far_tiled * (tile_y_idx + 1) * up + w_far_tiled * (tile_x_idx + 1) * right;
			Vector3 sub_far_down_left  = sub_far_origin + h_far_tiled * tile_y_idx * up + w_far_tiled * tile_x_idx * right;
			Vector3 sub_far_down_right = sub_far_origin + h_far_tiled * tile_y_idx * up + w_far_tiled * (tile_x_idx + 1) * right;

			// the store order is left-right-up-down
			Vector3 left_plane_n = (sub_far_down_left - sub_near_down_left).crossProduct(sub_near_top_left - sub_near_down_left);
			left_plane_n.normalize();
			Vector4 left_plane = Vector4(left_plane_n.getX(), left_plane_n.getY(), left_plane_n.getZ(), -(sub_near_down_left.m_x * left_plane_n.getX() + sub_near_down_left.m_y * left_plane_n.getY() + sub_near_down_left.m_z * left_plane_n.getZ()));

			Vector3 right_plane_n = (sub_near_top_right - sub_near_down_right).crossProduct(sub_far_down_right - sub_near_down_right);
			right_plane_n.normalize();
			Vector4 right_plane = Vector4(right_plane_n.getX(), right_plane_n.getY(), right_plane_n.getZ(), -(sub_near_down_right.m_x * right_plane_n.getX() + sub_near_down_right.m_y * right_plane_n.getY() + sub_near_down_right.m_z * right_plane_n.getZ()));

			Vector3 up_plane_n = (sub_far_top_left - sub_near_top_left).crossProduct(sub_near_top_right - sub_near_top_left);
			up_plane_n.normalize();
			Vector4 up_plane = Vector4(up_plane_n.getX(), up_plane_n.getY(), up_plane_n.getZ(), -(sub_near_top_left.m_x * up_plane_n.getX() + sub_near_top_left.m_y * up_plane_n.getY() + sub_near_top_left.m_z * up_plane_n.getZ()));

			Vector3 down_plane_n = (sub_near_down_right - sub_near_down_left).crossProduct(sub_far_down_left - sub_near_down_left);
			down_plane_n.normalize();
			Vector4 down_plane = Vector4(down_plane_n.getX(), down_plane_n.getY(), down_plane_n.getZ(), -(sub_near_down_left.m_x * down_plane_n.getX() + sub_near_down_left.m_y * down_plane_n.getY() + sub_near_down_left.m_z * down_plane_n.getZ()));
		
			m_SubFrustum[tileIndex * 4] = left_plane;
			m_SubFrustum[tileIndex * 4 + 1] = right_plane;
			m_SubFrustum[tileIndex * 4 + 2] = up_plane;
			m_SubFrustum[tileIndex * 4 + 3] = down_plane;
		}
	}
	
	SceneNode::do_CALCULATE_TRANSFORMATIONS(pEvt);

}

}; // namespace Components
}; // namespace PE
