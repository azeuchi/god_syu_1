#include "PlayerRenderer.h"
#include "DirectX.h"
#include "SceneBase.hpp"
#include "Player.h"
#include "Model.h"
#include "Shader.h"
#include "CameraBase.h"
#include "LightBase.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	// 位置（＋オフセット）・回転・スケールからワールド行列を組み、転置して返す
	void BuildWVP(Player* p, const XMFLOAT3& offset, XMFLOAT4X4& out)
	{
		XMFLOAT3 pos = p->GetPosition();
		XMFLOAT3 rot = p->GetRotation();
		XMFLOAT3 sc = p->GetScale();
		Matrix S = Matrix::CreateScale(sc.x, sc.y, sc.z);
		Matrix baseS = p->GetModel()->GetScaleBaseMatrix();
		Matrix R = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		Matrix T = Matrix::CreateTranslation(pos.x + offset.x, pos.y + offset.y, pos.z + offset.z);
		Matrix world = baseS * S * R * T;
		XMStoreFloat4x4(&out, XMMatrixTranspose(world));
	}
}

PlayerRenderer::~PlayerRenderer()
{
	if (m_pCullFront) { m_pCullFront->Release(); m_pCullFront = nullptr; }
	if (m_pCullBack) { m_pCullBack->Release(); m_pCullBack = nullptr; }
}

bool PlayerRenderer::Setup(SceneBase* scene)
{
	if (!scene) return false;

	// 描画に使うシェーダーを用意（既にあれば作らない）
	struct ShaderDef { const char* name; const char* file; bool isVS; };
	const ShaderDef defs[] = {
		{ "VS_SkinMeshAnimation", "Assets/Shader/VS_SkinMeshAnimation.cso", true },
		{ "PS_TexColor",          "Assets/Shader/PS_TexColor.cso",          false },
		{ "VS_SkinMeshOutline",   "Assets/Shader/VS_SkinMeshOutline.cso",   true },
		{ "PS_Outline",           "Assets/Shader/PS_Outline.cso",           false },
	};
	for (const auto& d : defs)
	{
		if (scene->GetObj<Shader>(d.name)) continue;
		Shader* sh = d.isVS
			? (Shader*)scene->CreateObj<VertexShader>(d.name)
			: (Shader*)scene->CreateObj<PixelShader>(d.name);
		sh->Load(d.file);
	}

	// 反転モデル（2P）にも対応するため表面／裏面カリングを用意
	D3D11_RASTERIZER_DESC rs = {};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
	rs.CullMode = D3D11_CULL_FRONT;
	GetDevice()->CreateRasterizerState(&rs, &m_pCullFront);
	rs.CullMode = D3D11_CULL_BACK;
	GetDevice()->CreateRasterizerState(&rs, &m_pCullBack);
	return true;
}

void PlayerRenderer::DrawOutline(SceneBase* scene, Player* player, const XMFLOAT3& offset)
{
	if (!player) return;
	Shader* vsOutline = scene->GetObj<Shader>("VS_SkinMeshOutline");
	Shader* psOutline = scene->GetObj<Shader>("PS_Outline");
	CameraBase* cam = scene->GetObj<CameraBase>("Camera");
	if (!vsOutline || !psOutline || !cam) return;

	// 反転時は裏面を描きたいので CullBack、通常は CullFront
	bool flipped = (player->GetScale().x < 0.0f);
	if (flipped) { if (m_pCullBack) GetContext()->RSSetState(m_pCullBack); }
	else { if (m_pCullFront) GetContext()->RSSetState(m_pCullFront); }

	XMFLOAT4X4 mat[3];
	BuildWVP(player, offset, mat[0]);
	mat[1] = cam->GetView();
	mat[2] = cam->GetProj();
	vsOutline->WriteBuffer(0, mat); // WVPのみ
	player->SetVertexShader(vsOutline);
	player->SetPixelShader(psOutline);
	player->Draw();
}

void PlayerRenderer::DrawBody(SceneBase* scene, Player* player, const XMFLOAT3& offset)
{
	if (!player) return;
	Shader* vs = scene->GetObj<Shader>("VS_SkinMeshAnimation");
	Shader* ps = scene->GetObj<Shader>("PS_TexColor");
	CameraBase* cam = scene->GetObj<CameraBase>("Camera");
	LightBase* light = scene->GetObj<LightBase>("Light");
	if (!vs || !ps || !cam || !light) return;

	// 反転時は表面を描きたいので CullFront、通常は CullBack
	bool flipped = (player->GetScale().x < 0.0f);
	if (flipped) { if (m_pCullFront) GetContext()->RSSetState(m_pCullFront); }
	else { if (m_pCullBack) GetContext()->RSSetState(m_pCullBack); }

	XMFLOAT4X4 mat[3];
	BuildWVP(player, offset, mat[0]);
	mat[1] = cam->GetView();
	mat[2] = cam->GetProj();

	XMFLOAT3 lightDir = light->GetDirection();
	XMFLOAT4 lightParam[] = {
		light->GetDiffuse(),
		light->GetAmbient(),
		{ lightDir.x, lightDir.y, lightDir.z, 0.0f }
	};
	XMFLOAT3 camPos = cam->GetPos();
	XMFLOAT4 camParam[] = { { camPos.x, camPos.y, camPos.z, 0.0f } };

	vs->WriteBuffer(0, mat);
	ps->WriteBuffer(0, lightParam);
	ps->WriteBuffer(1, camParam);
	player->SetVertexShader(vs);
	player->SetPixelShader(ps);
	player->Draw();
}