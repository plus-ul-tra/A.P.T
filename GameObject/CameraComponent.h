#pragma once
#include "Component.h"
#include "MathHelper.h"
#include "CameraSettings.h"
using namespace MathUtils;

class CameraComponent : public Component
{
	friend class Editor;

protected:
	// 내부 재계산
	void RebuildViewIfDirty    ();
	void RebuildProjIfDirty    ();

protected:
	// Viewport(또는 화면) 관련
	Viewport m_Viewport;

	float m_NearZ  = 1.0f;
	float m_FarZ   = 100.0f;

	PerspectiveParams m_Perspective;

	OrthoParams m_Ortho;

	OrthoOffCenterParams m_OrthoOffCenter;


	XMFLOAT4X4 m_View = Identity();
	XMFLOAT4X4 m_Proj = Identity();

	ProjectionMode m_Mode = ProjectionMode::Perspective;

	bool m_ViewDirty = true;
	bool m_ProjDirty = true;

	XMFLOAT3 m_Eye  { 0.0f, 3.0f, -5.0f };
	//Look : 방향벡터 X, Target Point
	XMFLOAT3 m_Look { 0.0f, 0.0f,  0.0f };
	XMFLOAT3 m_Up   { 0.0f, 1.0f,  0.0f };


public:
	CameraComponent() = default;
	CameraComponent(float viewportW, float viewportH,
		float fov, float aspect, float nearZ, float farZ, ProjectionMode mode);

	CameraComponent(float viewportW, float viewportH,
		float left, float right, float bottom, float top, float nearZ, float farZ);
	virtual ~CameraComponent() = default;

	static constexpr const char* StaticTypeName = "CameraComponent";
	const char* GetTypeName() const override;

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Deserialize(const nlohmann::json& j) override;

	// 화면 크기 들어오면 aspect 갱신
	void SetViewport(const Viewport& viewport);
	const Viewport& GetViewport() const
	{
		return m_Viewport;
	}

	void SetEye(const XMFLOAT3& eye) { m_Eye = eye;  m_ViewDirty = true; }
	const XMFLOAT3& GetEye() const     { return m_Eye; }

	void SetLook(const XMFLOAT3& look) { m_Look = look; m_ViewDirty = true;	}
	const XMFLOAT3& GetLook() const    { return m_Look; }

	void SetUp(const XMFLOAT3& up) { m_Up = up; m_ViewDirty = true; }
	const XMFLOAT3& GetUp  () const { return m_Up;   }

	void SetPerspective(const PerspectiveParams& persp) { m_Perspective = persp; m_ProjDirty = true;}
	const PerspectiveParams& GetPerspective() const		{ return m_Perspective;  }
	void SetOrtho(const OrthoParams& ortho) { m_Ortho = ortho; m_ProjDirty = true; }
	const OrthoParams& GetOrtho() const     { return m_Ortho;  }
	void SetOrthoOffCenter(const OrthoOffCenterParams& orthoOC) { m_OrthoOffCenter = orthoOC; m_ProjDirty = true; }
	const OrthoOffCenterParams& GetOrthoOffCenter() const		{ return m_OrthoOffCenter;    }

	void SetNearZ(const float& nearz) { if (nearz < 0.1f) { return; }  m_NearZ = nearz;  m_ProjDirty = true; }
	const float& GetNearZ() const { return m_NearZ; }

	void SetFarZ(const float& farz) { if (farz < 0.1f) { return; } m_FarZ = farz; m_ProjDirty = true; }
	const float& GetFarZ() const { return m_FarZ; }


	const ProjectionMode& GetProjectionMode() const { return m_Mode; }
	void SetProjectionMode(const ProjectionMode& mode);

	// Get할때만 계산해서 받아옴
	XMFLOAT4X4 GetViewMatrix  ();
	XMFLOAT4X4 GetProjMatrix  ();

	void SetEyeLookUp(const XMFLOAT3& eye, const XMFLOAT3& look, const XMFLOAT3& up);


	void SetPerspectiveProj   (const float fov, const float aspect, const float nearZ, const float farZ);
	void SetOrthoProj         (const float width, const float height, const float nearZ, const float farZ);
	void SetOrthoOffCenterProj(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);
};

