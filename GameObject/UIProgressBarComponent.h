#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include "ResourceHandle.h"
#include "UIImageComponent.h"
#include <functional>

class UIProgressBarComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIProgressBarComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetPercent(const float& percent);
	const float& GetPercent() const { return m_Percent; }

	void SetOnPercentChanged(std::function<void(float)> callback)
	{
		m_OnPercentChanged = std::move(callback);
	}

	void SetBackgroundTextureHandle(const TextureHandle& handle) { m_BackgroundTextureHandle = handle; }
	const TextureHandle& GetBackgroundTextureHandle() const { return m_BackgroundTextureHandle; }

	void SetBackgroundShaderAssetHandle(const ShaderAssetHandle& handle) { m_BackgroundShaderAssetHandle = handle; }
	const ShaderAssetHandle& GetBackgroundShaderAssetHandle() const { return m_BackgroundShaderAssetHandle; }

	void SetBackgroundVertexShaderHandle(const VertexShaderHandle& handle) { m_BackgroundVertexShaderHandle = handle; }
	const VertexShaderHandle& GetBackgroundVertexShaderHandle() const { return m_BackgroundVertexShaderHandle; }

	void SetBackgroundPixelShaderHandle(const PixelShaderHandle& handle) { m_BackgroundPixelShaderHandle = handle; }
	const PixelShaderHandle& GetBackgroundPixelShaderHandle() const { return m_BackgroundPixelShaderHandle; }

	void SetFillTextureHandle(const TextureHandle& handle) { m_FillTextureHandle = handle; }
	const TextureHandle& GetFillTextureHandle() const { return m_FillTextureHandle; }

	void SetFillMaskTextureHandle(const TextureHandle& handle) { m_FillMaskTextureHandle = handle; }
	const TextureHandle& GetFillMaskTextureHandle() const { return m_FillMaskTextureHandle; }

	void SetFillShaderAssetHandle(const ShaderAssetHandle& handle) { m_FillShaderAssetHandle = handle; }
	const ShaderAssetHandle& GetFillShaderAssetHandle() const { return m_FillShaderAssetHandle; }

	void SetFillVertexShaderHandle(const VertexShaderHandle& handle) { m_FillVertexShaderHandle = handle; }
	const VertexShaderHandle& GetFillVertexShaderHandle() const { return m_FillVertexShaderHandle; }

	void SetFillPixelShaderHandle(const PixelShaderHandle& handle) { m_FillPixelShaderHandle = handle; }
	const PixelShaderHandle& GetFillPixelShaderHandle() const { return m_FillPixelShaderHandle; }

	void SetFillDirection(const UIFillDirection& direction) { m_FillDirection = direction; }
	const UIFillDirection& GetFillDirection() const { return m_FillDirection; }

	void SetFillMode(const UIProgressFillMode& mode) { m_FillMode = mode; }
	const UIProgressFillMode& GetFillMode() const { return m_FillMode; }
private:
	float m_Percent = 0.0f;
	UIFillDirection m_FillDirection = UIFillDirection::LeftToRight;
	UIProgressFillMode m_FillMode = UIProgressFillMode::Rect;
	std::function<void(float)> m_OnPercentChanged;

	TextureHandle m_BackgroundTextureHandle = TextureHandle::Invalid();
	ShaderAssetHandle m_BackgroundShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_BackgroundVertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_BackgroundPixelShaderHandle = PixelShaderHandle::Invalid();
	TextureHandle m_FillTextureHandle = TextureHandle::Invalid();
	ShaderAssetHandle m_FillShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_FillVertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_FillPixelShaderHandle = PixelShaderHandle::Invalid();
	TextureHandle m_FillMaskTextureHandle = TextureHandle::Invalid();
};

