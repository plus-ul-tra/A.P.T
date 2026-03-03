#pragma once

enum class ProjectionMode
{
	Perspective,
	Orthographic,
	OrthoOffCenter
};

struct Viewport
{
	float Width = 0.0f;
	float Height = 0.0f;
};

struct PerspectiveParams
{
	float Fov = XM_PIDIV4;
	float Aspect = 1.0f;
};

struct OrthoParams
{
	float Width = 10.0f;
	float Height = 10.0f;
};

struct OrthoOffCenterParams
{
	float Left = -1.0f;
	float Right = 1.0f;
	float Bottom = -1.0f; ;
	float Top = 1.0f;
};