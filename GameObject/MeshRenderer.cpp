#include "MeshRenderer.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(MeshRenderer)
REGISTER_PROPERTY_READONLY_LOADABLE(MeshRenderer, Mesh)
REGISTER_PROPERTY_READONLY_LOADABLE(MeshRenderer, Material)
REGISTER_PROPERTY(MeshRenderer, Visible)
REGISTER_PROPERTY(MeshRenderer, CastShadow)
REGISTER_PROPERTY(MeshRenderer, ReceiveShadow)
REGISTER_PROPERTY(MeshRenderer, RenderLayer)

void MeshRenderer::SetRenderLayer(const UINT8& layer)
{
	m_RenderLayer = layer;

	auto* owner = GetOwner();
	if (!owner)
		return;

	owner->SetLayer(static_cast<RenderData::RenderLayer>(layer));
}

const UINT8& MeshRenderer::GetRenderLayer() const
{
	return m_RenderLayer;
}

bool MeshRenderer::BuildRenderItem(RenderData::RenderItem& out)
{
	if (!m_Visible)
		return false;

	MeshHandle     mesh     = MeshHandle::Invalid();
	MaterialHandle material = MaterialHandle::Invalid();
	ResolveHandles(mesh, material);

	if (!mesh.IsValid() || !material.IsValid())
		return false;

	TransformComponent* transform = GetTransform();
	if (!transform)
		return false;

	out.mesh     = mesh;
	out.material = material;
	out.world    = transform->GetWorldMatrix();
	out.sortKey  = BuildSortKey(mesh, material, m_RenderLayer);
	return true;
}

void MeshRenderer::Update(float deltaTime)
{
}

void MeshRenderer::OnEvent(EventType type, const void* data)
{
}


/*
	layer를 상위 8비트(56~63)에 넣음
	→ 렌더 레이어가 다른 것들은 무조건 먼저 분리됨

	material.id를 중간 28비트(28~55)에 넣음
	→ 동일 레이어 내에서는 머티리얼 기준으로 정렬

	mesh.id를 하위 28비트(0~27)에 넣음
	→ 같은 머티리얼이면 메쉬 기준 정렬

	정렬 우선 순위 : layer > material.id > mesh.id
*/
UINT64 MeshRenderer::BuildSortKey(MeshHandle mesh, MaterialHandle material, UINT8 layer)
{
	UINT key = 0;
	key |= (static_cast<UINT64>(layer) << 56);
	key |= (static_cast<UINT64>(material.id) << 28);
	key |=  static_cast<UINT64>(mesh.id);
	return key;
}

void MeshRenderer::ResolveHandles(MeshHandle& mesh, MaterialHandle& material)
{
	mesh     = m_MeshHandle;
	material = m_MaterialHandle;

	const Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}
	
	if (!mesh.IsValid())
	{
		if (const auto* meshComponent = owner->GetComponent<MeshComponent>())
		{
			mesh = meshComponent->GetMeshHandle();
			// 파생값 갱신 (읽기전용 표시용)
			MeshRef ref{};
			if (auto* loader = AssetLoader::GetActive())
				loader->GetMeshAssetReference(mesh, ref.assetPath, ref.assetIndex);

			LoadSetMesh(ref); // 또는 내부 갱신 함수
		}
	}

	if (!material.IsValid())
	{
		if (const auto* materialComponent = owner->GetComponent<MaterialComponent>())
		{
			material = materialComponent->GetMaterialHandle();
			// 파생값 갱신 (읽기전용 표시용)
			MaterialRef ref{};
			if (auto* loader = AssetLoader::GetActive())
				loader->GetMaterialAssetReference(material, ref.assetPath, ref.assetIndex);

			LoadSetMaterial(ref); // 또는 내부 갱신 함수
		}
	}
}
 
TransformComponent* MeshRenderer::GetTransform() const 
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return nullptr;
	}

	return owner->GetComponent<TransformComponent>();
}
