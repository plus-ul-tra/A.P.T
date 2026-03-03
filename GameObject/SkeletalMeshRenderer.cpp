#include "SkeletalMeshRenderer.h"
#include "TransformComponent.h"
#include "SkeletalMeshComponent.h"
#include "MaterialComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT_DERIVED(SkeletalMeshRenderer, MeshRenderer)
REGISTER_PROPERTY_READONLY_LOADABLE(SkeletalMeshRenderer, Skeleton)

bool SkeletalMeshRenderer::BuildRenderItem(RenderData::RenderItem& out)
{
	if (!m_Visible)
		return false;

	MeshHandle     mesh     = MeshHandle::Invalid();
	MaterialHandle material = MaterialHandle::Invalid();
	SkeletonHandle skeleton = SkeletonHandle::Invalid();
	ResolveHandles(mesh, material, skeleton);

	if (!mesh.IsValid() || !material.IsValid() || !skeleton.IsValid())
		return false;

	TransformComponent* transform = GetTransform();
	if (!transform)
		return false;

	out.mesh     = mesh;
	out.material = material;
	out.skeleton = skeleton;
	out.world    = transform->GetWorldMatrix();
	out.sortKey  = BuildSortKey(mesh, material, m_RenderLayer);
	return true;
}

void SkeletalMeshRenderer::Update(float deltaTime)
{
}

void SkeletalMeshRenderer::OnEvent(EventType type, const void* data)
{
}

void SkeletalMeshRenderer::ResolveHandles(MeshHandle& mesh, MaterialHandle& material, SkeletonHandle& skeleton)
{
	mesh = m_MeshHandle;
	material = m_MaterialHandle;
	skeleton = m_SkeletonHandle;

	const Object* owner = GetOwner();
	if (!owner)
		return;

	if (!mesh.IsValid())
	{
		if (const auto* skeletalMeshComp = owner->GetComponent<SkeletalMeshComponent>())
		{
			mesh = skeletalMeshComp->GetMeshHandle();

			// 파생값 갱신 (읽기전용 표시용)
			MeshRef ref{};
			if (auto* loader = AssetLoader::GetActive())
				loader->GetMeshAssetReference(mesh, ref.assetPath, ref.assetIndex);

			LoadSetMesh(ref); // 또는 내부 갱신 함수
		}
	}

	if (!material.IsValid())
	{
		if (const auto* materialComp = owner->GetComponent<MaterialComponent>())
		{
			material = materialComp->GetMaterialHandle();

			// 파생값 갱신 (읽기전용 표시용)
			MaterialRef ref{};
			if (auto* loader = AssetLoader::GetActive())
				loader->GetMaterialAssetReference(material, ref.assetPath, ref.assetIndex);

			LoadSetMaterial(ref); // 또는 내부 갱신 함수
		}
	}

	if (!skeleton.IsValid())
	{
		if (const auto* skeletalMeshComp = owner->GetComponent<SkeletalMeshComponent>())
		{
			skeleton = skeletalMeshComp->GetSkeletonHandle();
			// 파생값 갱신 (읽기전용 표시용)
			SkeletonRef ref{};
			if (auto* loader = AssetLoader::GetActive())
				loader->GetSkeletonAssetReference(skeleton, ref.assetPath, ref.assetIndex);

			LoadSetSkeleton(ref); // 또는 내부 갱신 함수
		}
	}
}