#include <Engine/Engine2D/Editor2D.hpp>
#include <Engine/Camera.hpp>

#include <im3d.h>
#include <im3d_math.h>

constexpr Im3d::Color Color_GizmoHighlight = Im3d::Color_Gold;

using namespace Im3d;

Im3d::Vec3 ToIm3d(const DG::float3& f) {
	return Im3d::Vec3(f.x, f.y, f.z);
}

Im3d::Vec2 ToIm3d(const DG::float2& f) {
	return Im3d::Vec2(f.x, f.y);
}

Im3d::Mat4 ToIm3d(const DG::float4x4& f) {
	return Im3d::Mat4(
		f.m00, f.m01, f.m02, f.m03,
		f.m10, f.m11, f.m12, f.m13,
		f.m20, f.m21, f.m22, f.m23,
		f.m30, f.m31, f.m32, f.m33);
}

namespace Morpheus {
	void Editor2D::Initialize(const Editor2DParams& params) {
		mInternalFont = params.mEditorFont;

		auto device = params.mEngine->GetDevice();
		auto swapChain = params.mEngine->GetSwapChain();

		Im3dRendererFactory factory;
		mIm3dGlobalsBuffer = std::make_unique<Im3dGlobalsBuffer>(device);

		factory.Initialize(params.mEngine->GetDevice(),
			mIm3dGlobalsBuffer.get(),
			swapChain->GetDesc().ColorBufferFormat,
			swapChain->GetDesc().DepthBufferFormat);

		mIm3dRenderer = std::make_unique<Im3dRenderer>(device, &factory);
	}

	void Editor2D::Update(Engine* engine, Scene* scene, float dt) {
		DG::float3 eye;
		DG::float3 lookAt;
		DG::float4x4 view;
		DG::float4x4 proj;
		DG::float4x4 viewProj;
		DG::float4x4 viewProjInv;

		auto swapChain = engine->GetSwapChain();

		Camera::ComputeTransformations(scene->GetCameraNode(),
			engine, &eye, &lookAt, &view, &proj, &viewProj);
		viewProjInv = viewProj.Inverse();

		auto& ad = Im3d::GetAppData();

		auto mouseState = engine->GetInputController().GetMouseState();
		auto& scDesc = swapChain->GetDesc();

		DG::float2 viewportSize((float)scDesc.Width, (float)scDesc.Height);

		ad.m_deltaTime     = dt;
		ad.m_viewportSize  = ToIm3d(viewportSize);
		ad.m_viewOrigin    = ToIm3d(eye); // for VR use the head position
		ad.m_viewDirection = ToIm3d(lookAt - eye);
		ad.m_worldUp       = Im3d::Vec3(0.0f, 1.0f, 0.0f); // used internally for generating orthonormal bases
		ad.m_projOrtho     = true;

		// m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
		ad.m_projScaleY = ad.m_projOrtho 
			? 2.0f / proj.m11 :
			tanf(scene->GetCamera()->GetFieldOfView() * 0.5f) * 2.0f; // or vertical fov for a perspective projection

		// World space cursor ray from mouse position; for VR this might be the position/orientation of the HMD or a tracked controller.
		DG::float2 cursorPos(mouseState.PosX, mouseState.PosY);
		cursorPos = 2.0f * cursorPos / viewportSize - DG::float2(1.0f, 1.0f);
		cursorPos.y = -cursorPos.y; // window origin is top-left, ndc is bottom-left

		DG::float4 rayDir = DG::float4(cursorPos.x, cursorPos.y, -1.0f, 1.0f) * viewProjInv; 
		rayDir = rayDir / rayDir.w;
		DG::float3 rayDirection = DG::float3(rayDir.x, rayDir.y, rayDir.z) - eye;
		rayDirection = DG::normalize(rayDirection);

		ad.m_cursorRayOrigin = ToIm3d(eye);
		ad.m_cursorRayDirection = ToIm3d(rayDirection);

		// Set cull frustum planes. This is only required if IM3D_CULL_GIZMOS or IM3D_CULL_PRIMTIIVES is enable via
		// im3d_config.h, or if any of the IsVisible() functions are called.
		ad.setCullFrustum(ToIm3d(viewProj), true);

		// Fill the key state array; using GetAsyncKeyState here but this could equally well be done via the window proc.
		// All key states have an equivalent (and more descriptive) 'Action_' enum.
		ad.m_keyDown[Im3d::Mouse_Left] = mouseState.ButtonFlags & mouseState.BUTTON_FLAG_LEFT;

		// Enable gizmo snapping by setting the translation/rotation/scale increments to be > 0
		ad.m_snapTranslation = 0.0f;
		ad.m_snapRotation    = 0.0f;
		ad.m_snapScale       = 0.0f;

		mContext.reset();
		GizmoTranslation(MakeId("Gizmo"), mTranslation, false, mContext);
		mContext.endFrame();
	}

	void Editor2D::Render(Engine* engine, Scene* scene, DG::IDeviceContext* context) {
		mIm3dGlobalsBuffer->Write(context, scene->GetCameraNode(), engine);
		mIm3dRenderer->Draw(context, &mContext);
	}

	void Editor2D::RenderUI(Engine* engine, Scene* scene) {

		if (mInternalFont)
			ImGui::PushFont(mInternalFont);

		if (ImGui::BeginMainMenuBar()) {

			if (ImGui::BeginMenu("File", true)) {

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (mInternalFont)
			ImGui::PopFont();
	}

	bool Editor2D::GizmoTranslation(Id _id, float _translation_[3], bool _local, Context& context)
	{
		Context& ctx = context;

		bool ret = false;
		Vec3* outVec3 = (Vec3*)_translation_;
		Vec3 drawAt = *outVec3;
		const AppData& appData = ctx.getAppData();

		float worldHeight = ctx.pixelsToWorldSize(drawAt, ctx.m_gizmoHeightPixels);

		ctx.pushId(_id);
		ctx.m_appId = _id;

		if (_local)
		{
			Mat4 localMatrix = ctx.getMatrix();
			localMatrix.setScale(Vec3(1.0f));
			ctx.pushMatrix(localMatrix);
		}

		float planeSize = worldHeight * (0.5f * 0.5f);
		float planeOffset = worldHeight * 0.5f;
		float worldSize = ctx.pixelsToWorldSize(drawAt, ctx.m_gizmoSizePixels);

		struct AxisG { Id m_id; Vec3 m_axis; Color m_color; };
		AxisG axes[] =
			{
				{ MakeId("axisX"), Vec3(1.0f, 0.0f, 0.0f), Color_Red   },
				{ MakeId("axisY"), Vec3(0.0f, 1.0f, 0.0f), Color_Green }
			};
		struct PlaneG { Id m_id; Vec3 m_origin; };

		PlaneG planes[] =
			{
				{ MakeId("planeXY"), Vec3(planeOffset, planeOffset, 0.0f) },
				{ MakeId("planeV"),  Vec3(0.0f, 0.0f, 0.0f) }
			};

	// invert axes if viewing from behind
		if (appData.m_flipGizmoWhenBehind)
		{
			const Vec3 viewDir = appData.m_projOrtho
				? -appData.m_viewDirection
				: Normalize(appData.m_viewOrigin - *outVec3)
				;
			for (int i = 0; i < 2; ++i)
			{
				const Vec3 axis = _local ? Vec3(ctx.getMatrix().getCol(i)) : axes[i].m_axis;
				if (Dot(axis, viewDir) < 0.0f)
				{
					axes[i].m_axis = -axes[i].m_axis;
					for (int j = 0; j < 1; ++j)
					{
						planes[j].m_origin[i] = -planes[j].m_origin[i];
					}
				}
			}
		}

		Sphere boundingSphere(*outVec3, worldHeight * 1.5f); // expand the bs to catch the planar subgizmos
		Ray ray(appData.m_cursorRayOrigin, appData.m_cursorRayDirection);
		bool intersects = ctx.m_appHotId == ctx.m_appId || Intersects(ray, boundingSphere);

	// planes
		ctx.pushEnableSorting(true);
		if (_local)
		{
		// local planes need to be drawn with the pushed matrix for correct orientation
			for (int i = 0; i < 1; ++i)
			{
				const PlaneG& plane = planes[i];
				ctx.gizmoPlaneTranslation_Draw(plane.m_id, plane.m_origin, axes[i].m_axis, planeSize, Color_GizmoHighlight);
				axes[i].m_axis = Mat3(ctx.getMatrix()) * axes[i].m_axis;
				if (intersects)
				{
					ret |= ctx.gizmoPlaneTranslation_Behavior(plane.m_id, ctx.getMatrix() * plane.m_origin, axes[i].m_axis, appData.m_snapTranslation, planeSize, outVec3);
				}
			}
		}
		else
		{
			ctx.pushMatrix(Mat4(1.0f));
			for (int i = 0; i < 1; ++i)
			{
				const PlaneG& plane = planes[i];
				ctx.gizmoPlaneTranslation_Draw(plane.m_id, drawAt + plane.m_origin, axes[i].m_axis, planeSize, Color_GizmoHighlight);
				if (intersects)
				{
					ret |= ctx.gizmoPlaneTranslation_Behavior(plane.m_id, drawAt + plane.m_origin, axes[i].m_axis, appData.m_snapTranslation, planeSize, outVec3);
				}
			}
			ctx.popMatrix();
		}

		ctx.pushMatrix(Mat4(1.0f));

		if (intersects)
		{
		// view plane (store the normal when the gizmo becomes active)
			Id currentId = ctx.m_activeId;
			Vec3& storedViewNormal= *((Vec3*)ctx.m_gizmoStateMat3.m);
			Vec3 viewNormal;
			if (planes[1].m_id == ctx.m_activeId)
			{
				viewNormal = storedViewNormal;
			}
			else
			{
				viewNormal = ctx.getAppData().m_viewDirection;
			}
			ret |= ctx.gizmoPlaneTranslation_Behavior(planes[1].m_id, drawAt, viewNormal, appData.m_snapTranslation, worldSize, outVec3);
			if (currentId != ctx.m_activeId)
			{
			// gizmo became active, store the view normal
				storedViewNormal = viewNormal;
			}

		// highlight axes if the corresponding plane is hot
			if (planes[0].m_id == ctx.m_hotId) // XY
			{
				axes[0].m_color = axes[1].m_color = Color_GizmoHighlight;
			}
			else if (planes[1].m_id == ctx.m_hotId) // view plane
			{
				axes[0].m_color = axes[1].m_color = Color_GizmoHighlight;
			}
		}
	// draw the view plane handle
		ctx.begin(PrimitiveMode_Points);
			ctx.vertex(drawAt, ctx.m_gizmoSizePixels * 2.0f, planes[1].m_id == ctx.m_hotId ? Color_GizmoHighlight : Color_White);
		ctx.end();

	// axes
		for (int i = 0; i < 2; ++i)
		{
			AxisG& axis = axes[i];
			ctx.gizmoAxisTranslation_Draw(axis.m_id, drawAt, axis.m_axis, worldHeight, worldSize, axis.m_color);
			if (intersects)
			{
				ret |= ctx.gizmoAxisTranslation_Behavior(axis.m_id, drawAt, axis.m_axis, appData.m_snapTranslation, worldHeight, worldSize, outVec3);
			}
		}
		ctx.popMatrix();
		ctx.popEnableSorting();

		if (_local)
		{
			ctx.popMatrix();
		}

		ctx.popId();

		return ret;
	}
}