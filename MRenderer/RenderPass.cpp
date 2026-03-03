
#include "RenderPass.h"

void RenderPass::Setup(const RenderData::FrameData& frame)
{
	BuildQueue(frame);
}



void RenderPass::SetBlendState(BS state)
{
	//if (state == BS::MAX_) // 예시: MAX_를 '기본 상태' 용도로 쓰고 싶다면
	//{
	//	m_RenderContext.pDXDC->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	//	return;
	//}
	m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[state].Get(), nullptr, 0xFFFFFFFF);
}

void RenderPass::SetDepthStencilState(DS state)
{
	m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[state].Get(), 0);
}

void RenderPass::SetRasterizerState(RS state)
{
	m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[state].Get());
}

void RenderPass::SetSamplerState()
{
	m_RenderContext.pDXDC->PSSetSamplers(0, 1, m_RenderContext.SState[SS::WRAP].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(1, 1, m_RenderContext.SState[SS::MIRROR].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(2, 1, m_RenderContext.SState[SS::CLAMP].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(3, 1, m_RenderContext.SState[SS::BORDER].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(4, 1, m_RenderContext.SState[SS::BORDER_SHADOW].GetAddressOf());
}

//클리어까지 동시 실행
void RenderPass::SetRenderTarget(ID3D11RenderTargetView* rtview, ID3D11DepthStencilView* dsview, const FLOAT* clearColor)
{
	//float clearColor[4] = { 0.21f, 0.21f, 0.21f, 1.f };

	if (rtview)
	{
		m_RenderContext.pDXDC->OMSetRenderTargets(1, &rtview, dsview);
		m_RenderContext.pDXDC->ClearRenderTargetView(rtview, clearColor);
	}
	else
	{
		m_RenderContext.pDXDC->OMSetRenderTargets(0, nullptr, dsview);
	}

	if (dsview)
	{
		m_RenderContext.pDXDC->ClearDepthStencilView(
			dsview,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			1.0f,
			0
		);
	}
}

void RenderPass::SetBaseCB(const RenderData::RenderItem& item)
{
	XMMATRIX mtm = XMLoadFloat4x4(&item.world);
	XMMATRIX mLocalToWorld = XMLoadFloat4x4(&item.localToWorld);

	mtm = MathUtils::Mul(mLocalToWorld, mtm);

	XMFLOAT4X4 tm;
	XMStoreFloat4x4(&tm, mtm);

	m_RenderContext.BCBuffer.mWorld = tm;

	m_RenderContext.BCBuffer.ScreenSize.x = m_RenderContext.WindowSize.width;
	m_RenderContext.BCBuffer.ScreenSize.y = m_RenderContext.WindowSize.height;

	XMMATRIX world = XMLoadFloat4x4(&tm);
	XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
	XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorldInvTranspose, worldInvTranspose);


	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));

}

XMMATRIX BuildMaskTM(
	const XMVECTOR& camPos,
	const XMFLOAT3& enemyPos)
{
	XMVECTOR look = XMLoadFloat3(&enemyPos);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	if (XMVector3Equal(camPos, look))
		return XMMatrixIdentity();

	// look ↔ camPos 교체
	XMMATRIX view = XMMatrixLookAtLH(look, camPos, up);
	XMMATRIX proj = XMMatrixOrthographicLH(8, 8, 0.1f, 200.f);

	static const XMMATRIX texScale = XMMatrixSet(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);

	return view * proj * texScale;
}



//벽뚫 마스킹맵용 행렬
void RenderPass::SetMaskingTM(const RenderData::FrameData& frame, const XMFLOAT3& campos)
{
	if (!m_RenderContext.isEditCam)
	{
		XMMATRIX mTM;

		XMVECTOR maincampos = XMLoadFloat3(&campos); 
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		//전체 초기화
		XMStoreFloat4x4(
			&m_RenderContext.MaskBuffer.PlayerMask,
			XMMatrixIdentity());

		for (int i = 0; i < enemyMaskSize; ++i)
		{
			XMStoreFloat4x4(
				&m_RenderContext.MaskBuffer.EnemyMask[i],
				XMMatrixIdentity());
		}


		//버퍼 채워넣기
		mTM = BuildMaskTM(maincampos, frame.playerPosition);

		XMStoreFloat4x4(&m_RenderContext.MaskBuffer.PlayerMask, mTM);



		int count = min(
			(int)frame.combatEnemyPositions.size(),
			enemyMaskSize
		);
		for (int i = 0; i < count; i++)
		{
			XMMATRIX mTM = BuildMaskTM(maincampos, frame.combatEnemyPositions[i]);

			XMStoreFloat4x4(
				&m_RenderContext.MaskBuffer.EnemyMask[i],
				mTM);
		}

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pMaskB.Get(), &(m_RenderContext.MaskBuffer), sizeof(m_RenderContext.MaskBuffer));
	}
}


void RenderPass::SetCameraCB(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

	if (!m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.gameCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.gameCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.gameCamera.viewProj;
		m_RenderContext.CameraCBuffer.camPos = context.gameCamera.cameraPos;


	}
	else if (m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.editorCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.editorCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.editorCamera.viewProj;
		m_RenderContext.CameraCBuffer.camPos = context.editorCamera.cameraPos;
	}

	//스카이박스 행렬
#pragma region SkyBox
	XMFLOAT4X4 view, proj;
	view = m_RenderContext.CameraCBuffer.mView;
	proj = m_RenderContext.CameraCBuffer.mProj;
	view._41 = view._42 = view._43 = 0.0f;

	XMMATRIX mV, mP, mVP, mSkyBox;
	mV = XMLoadFloat4x4(&view);
	mP = XMLoadFloat4x4(&proj);

	mVP = mV * mP;
	mSkyBox = XMMatrixInverse(nullptr, mVP);
	XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mSkyBox, mSkyBox);
	//스카이박스 행렬 끝

#pragma endregion
	m_RenderContext.CameraCBuffer.dTime = *m_RenderContext.dTime;

	m_RenderContext.camParams.x = context.gameCamera.camNear;
	m_RenderContext.camParams.y = context.gameCamera.camFar;
	m_RenderContext.camParams.w = 21.f;				//★★★★★★★★★★★★★★★★★★★★★★★★★★★★★

	m_RenderContext.CameraCBuffer.camParams = m_RenderContext.camParams;


	//초점
	float focusY = frame.playerPosition.y + 1.8f;

	// 카메라 위치 가져오기
	XMFLOAT3 camPos = m_RenderContext.CameraCBuffer.camPos;

	// 새로운 초점 위치 생성
	XMVECTOR focusPos = XMVectorSet(
		camPos.x + 0.5f,      // X → 카메라 기준
		focusY,        // Y → 플레이어 기준
		camPos.z + 3.0f,      // Z → 카메라 기준
		1.0f
	);

	// 월드 행렬
	XMMATRIX mWorld = XMMatrixTranslationFromVector(focusPos);

	// VP
	mVP = XMLoadFloat4x4(&m_RenderContext.CameraCBuffer.mVP);
	XMMATRIX WVP = mWorld * mVP;

	// 로컬 기준점
	XMVECTOR localPos = XMVectorSet(0.f, 0.f, 0.f, 1.f);

	// 클립 좌표
	XMVECTOR clipPos = XMVector4Transform(localPos, WVP);

	XMFLOAT4 clip;
	XMStoreFloat4(&clip, clipPos);

	// NDC
	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;

	float playerU = ndcX * 0.5f + 0.5f;
	float playerV = -ndcY * 0.5f + 0.5f;


	// View Space Z 계산 (DoF 초점용)
	XMMATRIX mView = XMLoadFloat4x4(&m_RenderContext.CameraCBuffer.mView);
	XMMATRIX mWV = mWorld * mView;

	XMVECTOR viewPos = XMVector3TransformCoord(localPos, mWV);
	float focusZ = XMVectorGetZ(viewPos);

	// 최종 초점 거리
	m_RenderContext.camParams.z = focusZ;

	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));
}

void RenderPass::SetDirLight(const RenderData::FrameData& frame)
{
	if (frame.currScene == 0 || frame.currScene == 3)
	{
		m_RenderContext.LightCBuffer.blurOn = 0;
	}
	else
	{
		m_RenderContext.LightCBuffer.blurOn = 1;
	}

	if (!frame.lights.empty())
	{
		for (const auto& light : frame.lights)
		{
			if (light.type != RenderData::LightType::Directional)
				continue;
			m_RenderContext.LightCBuffer.lightCount = 1;
			XMFLOAT4X4 view;
			if (m_RenderContext.isEditCam)
			{
				view = frame.context.editorCamera.view;
			}
			else if (!m_RenderContext.isEditCam)
			{
				view = frame.context.gameCamera.view;
			}
			XMMATRIX mView = XMLoadFloat4x4(&view);

			Light dirlight{};
			dirlight.worldDir = XMFLOAT3(-light.direction.x, -light.direction.y, -light.direction.z);

			XMVECTOR dirW = XMLoadFloat3(&dirlight.worldDir);
			XMVECTOR dirV = XMVector3Normalize(XMVector3TransformNormal(dirW, mView));
			XMStoreFloat3(&dirlight.viewDir, dirV);

			dirlight.Color = XMFLOAT4(light.color.x, light.color.y, light.color.z, 1);
			dirlight.Intensity = light.intensity;
			dirlight.Contrast = light.contrast;
			dirlight.Saturation = light.saturation;
			dirlight.mLightViewProj = light.lightViewProj;
			dirlight.CastShadow = light.castShadow;
			dirlight.Range = light.range;
			dirlight.SpotInnerAngle = light.spotInnerAngle;
			dirlight.SpotOutterAngle = light.spotOutterAngle;
			dirlight.AttenuationRadius = light.attenuationRadius;
			dirlight.type = static_cast<UINT>(light.type);

			m_RenderContext.LightCBuffer.lights[0] = dirlight;
			break;
		}

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pLightCB.Get(), &m_RenderContext.LightCBuffer, sizeof(LightConstBuffer));

	}

}

void RenderPass::SetOtherLights(const RenderData::FrameData& frame)
{
	if (!frame.lights.empty())
	{
		int index = 1;
		for (const auto& light : frame.lights)
		{
			if (light.type == RenderData::LightType::Directional)
				continue;
			m_RenderContext.LightCBuffer.lightCount++;
			
			//포인트 라이트
			if (light.type == RenderData::LightType::Point)
			{
				Light pointlight{};

				pointlight.Pos = light.posiiton;
				pointlight.Color = XMFLOAT4(light.color.x, light.color.y, light.color.z, 1);
				pointlight.Intensity = light.intensity;
				pointlight.Range = light.range;
				pointlight.type = static_cast<UINT>(light.type);

				m_RenderContext.LightCBuffer.lights[index] = pointlight;

			}

			//SpotLight
			if (light.type == RenderData::LightType::Spot)
			{
				XMFLOAT4X4 view;
				if (m_RenderContext.isEditCam)
				{
					view = frame.context.editorCamera.view;
				}
				else if (!m_RenderContext.isEditCam)
				{
					view = frame.context.gameCamera.view;
				}
				XMMATRIX mView = XMLoadFloat4x4(&view);

				Light spotlight{};

				spotlight.Pos = light.posiiton;
				spotlight.Color = XMFLOAT4(light.color.x, light.color.y, light.color.z, 1);
				spotlight.Intensity = light.intensity;
				spotlight.worldDir = light.direction;
				//spotlight.worldDir = XMFLOAT3(-light.direction.x, -light.direction.y, -light.direction.z);

				XMVECTOR dirW = XMLoadFloat3(&spotlight.worldDir);
				XMVECTOR dirV = XMVector3Normalize(XMVector3TransformNormal(dirW, mView));
				XMStoreFloat3(&spotlight.viewDir, dirV);

				spotlight.Range = light.range;
				spotlight.SpotInnerAngle = DirectX::XMConvertToRadians(light.spotInnerAngle);
				spotlight.SpotOutterAngle = DirectX::XMConvertToRadians(light.spotOutterAngle);	
				spotlight.AttenuationRadius = light.attenuationRadius;
				spotlight.type = static_cast<UINT>(light.type);

				m_RenderContext.LightCBuffer.lights[index] = spotlight;

			}


			index++;
		}

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pLightCB.Get(), &m_RenderContext.LightCBuffer, sizeof(LightConstBuffer));

	}
}

void RenderPass::SetMaterialCB(const RenderData::MaterialData& mat)
{
	//머티리얼 버퍼 업데이트
	m_RenderContext.MatBuffer.Color = mat.baseColor;
	m_RenderContext.MatBuffer.saturation = mat.saturation;
	m_RenderContext.MatBuffer.lightness = mat.lightness;
	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pMatB.Get(), &m_RenderContext.MatBuffer, sizeof(MaterialBuffer));
	m_RenderContext.pDXDC.Get()->VSSetConstantBuffers(5, 1, m_RenderContext.pMatB.GetAddressOf());
	m_RenderContext.pDXDC.Get()->PSSetConstantBuffers(5, 1, m_RenderContext.pMatB.GetAddressOf());

}

void RenderPass::SetVertex(const RenderData::RenderItem& item)
{
	const auto* vertexBuffers = m_RenderContext.vertexBuffers;
	const auto* indexBuffers = m_RenderContext.indexBuffers;
	const auto* indexCounts = m_RenderContext.indexCounts;
	const auto* textures = m_RenderContext.textures;

	if (vertexBuffers && indexBuffers && indexCounts && item.mesh.IsValid())
	{
		const MeshHandle bufferHandle = item.mesh;
		const auto vbIt = vertexBuffers->find(bufferHandle);
		const auto ibIt = indexBuffers->find(bufferHandle);
		const auto countIt = indexCounts->find(bufferHandle);

		if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexCounts->end())
		{
			ID3D11Buffer* vb = vbIt->second.Get();
			ID3D11Buffer* ib = ibIt->second.Get();
			const UINT32 fullCount = countIt->second;
			const bool useSubMesh = item.useSubMesh;
			const UINT32 indexCount = useSubMesh ? item.indexCount : fullCount;
			const UINT32 indexStart = useSubMesh ? item.indexStart : 0;
		}
	}
}

void RenderPass::DrawMesh(
	ID3D11Buffer* vb,
	ID3D11Buffer* ib,
	ID3D11VertexShader* vs,
	ID3D11PixelShader* ps,
	BOOL useSubMesh,
	UINT indexCount,
	UINT indexStart
)
{
	const UINT stride = sizeof(RenderData::Vertex);
	const UINT offset = 0;

	ID3D11DeviceContext* dc = m_RenderContext.pDXDC.Get();

	dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	dc->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
	dc->IASetInputLayout(m_RenderContext.InputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dc->VSSetShader(vs, nullptr, 0);
	dc->PSSetShader(ps, nullptr, 0);

	dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->VSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
	dc->PSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
	dc->VSSetConstantBuffers(4, 1, m_RenderContext.pMaskB.GetAddressOf());
	dc->PSSetConstantBuffers(4, 1, m_RenderContext.pMaskB.GetAddressOf());


	if (useSubMesh)
	{
		dc->DrawIndexed(indexCount, indexStart, 0);
	}
	else
	{
		dc->DrawIndexed(indexCount, 0, 0);
	}
}

void RenderPass::DrawBones(ID3D11VertexShader* vs, ID3D11PixelShader* ps, UINT boneCount)
{
	ID3D11DeviceContext* dc = m_RenderContext.pDXDC.Get();

	// VB/IB 안 씀
	dc->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	// InputLayout 필요 없음 (SV_VertexID만 사용)
	dc->IASetInputLayout(nullptr);

	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dc->VSSetShader(vs, nullptr, 0);
	dc->PSSetShader(ps, nullptr, 0);

	// 기존 CB들
	dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->VSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
	dc->PSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());

	// 본 N개 -> 라인 버텍스 2N개
	dc->Draw(boneCount * 2, 0);
}

bool RenderPass::ShouldIncludeRenderItem(RenderData::RenderLayer /*layer*/, const RenderData::RenderItem& /*item*/) const 
{
	return false;
}

void RenderPass::BuildQueue(const RenderData::FrameData& frame)
{
	m_Queue.clear();
	size_t totalItems = 0;
	for (const auto& [layer, items] : frame.renderItems)
	{
		totalItems += items.size();
	}
	m_Queue.reserve(totalItems);


	for (const auto& [layer, items] : frame.renderItems) 
	{
		for (const auto& item : items) 
		{
			if (ShouldIncludeRenderItem(layer, item))
			{
				m_Queue.push_back({ layer, &item });
			}
		}
	}
}
