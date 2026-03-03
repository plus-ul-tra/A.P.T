#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include "ResourceHandle.h"
#include <functional>

class UISliderComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UISliderComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetRange(const float& minValue, const float& maxValue);
	void SetMinValue(const float& minValue) { SetRange(minValue, m_MaxValue); }
	void SetMaxValue(const float& maxValue) { SetRange(m_MinValue, maxValue); }
	void SetValue(const float& value);
	const float& GetValue() const { return m_Value; }
	const float& GetMinValue() const { return m_MinValue; }
	const float& GetMaxValue() const { return m_MaxValue; }

	void SetNormalizedValue(float normalizedValue);
	float GetNormalizedValue() const;

	const bool& GetIsDragging() const { return m_IsDragging; }

	void SetOnValueChanged(std::function<void(float)> callback)
	{
		m_OnValueChanged = std::move(callback);
	}

	void HandleDrag(float normalizedValue);
	void HandleReleased();

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

	void SetFillShaderAssetHandle(const ShaderAssetHandle& handle) { m_FillShaderAssetHandle = handle; }
	const ShaderAssetHandle& GetFillShaderAssetHandle() const { return m_FillShaderAssetHandle; }

	void SetFillVertexShaderHandle(const VertexShaderHandle& handle) { m_FillVertexShaderHandle = handle; }
	const VertexShaderHandle& GetFillVertexShaderHandle() const { return m_FillVertexShaderHandle; }

	void SetFillPixelShaderHandle(const PixelShaderHandle& handle) { m_FillPixelShaderHandle = handle; }
	const PixelShaderHandle& GetFillPixelShaderHandle() const { return m_FillPixelShaderHandle; }

	void SetHandleTextureHandle(const TextureHandle& handle) { m_HandleTextureHandle = handle; }
	const TextureHandle& GetHandleTextureHandle() const { return m_HandleTextureHandle; }

	void SetHandleShaderAssetHandle(const ShaderAssetHandle& handle) { m_HandleShaderAssetHandle = handle; }
	const ShaderAssetHandle& GetHandleShaderAssetHandle() const { return m_HandleShaderAssetHandle; }

	void SetHandleVertexShaderHandle(const VertexShaderHandle& handle) { m_HandleVertexShaderHandle = handle; }
	const VertexShaderHandle& GetHandleVertexShaderHandle() const { return m_HandleVertexShaderHandle; }

	void SetHandlePixelShaderHandle(const PixelShaderHandle& handle) { m_HandlePixelShaderHandle = handle; }
	const PixelShaderHandle& GetHandlePixelShaderHandle() const { return m_HandlePixelShaderHandle; }

	void SetFillDirection(const UIFillDirection& direction) { m_FillDirection = direction; }
	const UIFillDirection& GetFillDirection() const { return m_FillDirection; }

	void SetHandleSizeOverride(const float& size) { m_HandleSizeOverride = size; }
	const float& GetHandleSizeOverride() const { return m_HandleSizeOverride; }

	bool HasHandleSizeOverride() const { return m_HandleSizeOverride > 0.0f; }

private:
	float m_MinValue = 0.0f;
	float m_MaxValue = 1.0f;
	float m_Value = 0.0f;
	bool m_IsDragging = false;
	std::function<void(float)> m_OnValueChanged;

	UIFillDirection m_FillDirection = UIFillDirection::LeftToRight;
	float m_HandleSizeOverride = 0.0f;

	TextureHandle m_BackgroundTextureHandle = TextureHandle::Invalid();
	ShaderAssetHandle m_BackgroundShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_BackgroundVertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_BackgroundPixelShaderHandle = PixelShaderHandle::Invalid();
	TextureHandle m_FillTextureHandle = TextureHandle::Invalid();
	ShaderAssetHandle m_FillShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_FillVertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_FillPixelShaderHandle = PixelShaderHandle::Invalid();
	TextureHandle m_HandleTextureHandle = TextureHandle::Invalid();
	ShaderAssetHandle m_HandleShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_HandleVertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_HandlePixelShaderHandle = PixelShaderHandle::Invalid();
};

