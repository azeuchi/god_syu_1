#include "Model.h"
#include "Defines.h"
#include "DebugLog.h"

#include <assimp/postprocess.h>
#include <unordered_set>

#if _MSC_VER >= 1920
#ifdef _DEBUG
#pragma comment(lib, "assimp/x64/Debug/assimp-vc142-mtd.lib")
#else
#pragma comment(lib, "assimp/x64/Release/assimp-vc142-mt.lib")
#endif
#elif _MSC_VER >= 1910
#ifdef _DEBUG
#pragma comment(lib, "assimp/x64/Debug/assimp-vc141-mtd.lib")
#endif
#endif

const DirectX::SimpleMath::Matrix DirectX::SimpleMath::Matrix::Identity = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

std::shared_ptr<VertexShader> Model::m_defVS = nullptr;
std::shared_ptr<PixelShader> Model::m_defPS = nullptr;

Model::Model()
	: m_pVS(nullptr)
	, m_pPS(nullptr)
{
	if (!m_defVS && !m_defPS)
	{
		MakeDefaultShader();
	}
	m_pVS = m_defVS.get();
	m_pPS = m_defPS.get();
	importer = new Assimp::Importer();
}
Model::~Model() {
	delete importer;
	for (auto& anim : m_Animation)
	{
		aiReleaseImport(anim.second);
	}
	m_Animation.clear();
}

void Model::SetVertexShader(Shader* vs)
{
	VertexShader* cast = reinterpret_cast<VertexShader*>(vs);
	if (cast)
		m_pVS = cast;
}
void Model::SetPixelShader(Shader* ps)
{
	PixelShader* cast = reinterpret_cast<PixelShader*>(ps);
	if (cast)
		m_pPS = cast;
}

const Model::Mesh* Model::GetMesh(unsigned int index) const
{
	if (index < 0 || m_meshes.size() <= index)
	{
		return nullptr;
	}
	return &m_meshes[index];
}

uint32_t Model::GetMeshNum() const
{
	return static_cast<uint32_t>(m_meshes.size());
}

bool Model::Load(const char* file, float scaleBase, bool flip, bool simpleMode)
{
	DebugLog::log(DebugLog::INFO_LOG, "モデル読み込み開始");
	DebugLog::log(DebugLog::INFO_LOG, "path ", file);
	m_scaleBase = scaleBase;

	int flag = 0;
	if (simpleMode)
	{
		flag |= aiProcess_Triangulate;
		flag |= aiProcess_JoinIdenticalVertices;
		flag |= aiProcess_FlipUVs;
		flag |= aiProcess_PreTransformVertices;
		if (flip) flag |= aiProcess_MakeLeftHanded;
	}
	else
	{
		flag |= aiProcessPreset_TargetRealtime_MaxQuality;
		flag |= aiProcess_PopulateArmatureData;
		if (flip) flag |= aiProcess_ConvertToLeftHanded;
	}

	m_pScene = importer->ReadFile(file, flag);
	if (!m_pScene) {
		Error(importer->GetErrorString());
		DebugLog::log(DebugLog::INFO_LOG, "Assimpモデルロード失敗", file);
		return false;
	}
	assert(m_pScene);

	DebugLog::log(DebugLog::INFO_LOG, "ボーン情報配列準備");
	CreateBone(m_pScene->mRootNode);

	unsigned int num = 0;
	for (auto& data : m_Bone) {
		data.second.idx = num;
		num++;
	}

	aiVector3D zero(0.0f, 0.0f, 0.0f);
	for (unsigned int i = 0; i < m_pScene->mNumMeshes; ++i)
	{
		Mesh mesh = {};
		std::vector<Vertex> vtx;
		vtx.resize(m_pScene->mMeshes[i]->mNumVertices);
		for (unsigned int j = 0; j < vtx.size(); ++j)
		{
			aiVector3D pos = m_pScene->mMeshes[i]->mVertices[j];
			aiVector3D uv = m_pScene->mMeshes[i]->HasTextureCoords(0) ?
				m_pScene->mMeshes[i]->mTextureCoords[0][j] : zero;
			aiVector3D normal = m_pScene->mMeshes[i]->HasNormals() ?
				m_pScene->mMeshes[i]->mNormals[j] : zero;
			vtx[j] = {
				DirectX::XMFLOAT3(pos.x, pos.y, pos.z),
				DirectX::XMFLOAT3(normal.x, normal.y, normal.z),
				DirectX::XMFLOAT2(uv.x, uv.y),
				{-1,-1,-1,-1},
				{0.0f,0.0f,0.0f,0.0f}
			};
		}

		std::vector<BONE> boneList = GetBoneInfo(m_pScene->mMeshes[i]);
		for (auto& bone : boneList)
		{
			for (auto& w : bone.weights)
			{
				int& idx = vtx[w.vertexindex].boneCount;
				vtx[w.vertexindex].boneIndex[idx] = m_Bone[w.bonename].idx;
				vtx[w.vertexindex].boneWeight[idx] = w.weight;
				idx++;
				assert(idx <= 4);
			}
		}

		std::vector<unsigned int> idx;
		idx.resize(m_pScene->mMeshes[i]->mNumFaces * 3);
		for (unsigned int j = 0; j < m_pScene->mMeshes[i]->mNumFaces; ++j)
		{
			aiFace face = m_pScene->mMeshes[i]->mFaces[j];
			int faceIdx = j * 3;
			idx[faceIdx + 0] = face.mIndices[0];
			idx[faceIdx + 1] = face.mIndices[1];
			idx[faceIdx + 2] = face.mIndices[2];
		}

		mesh.materialID = m_pScene->mMeshes[i]->mMaterialIndex;

		MeshBuffer::Description desc = {};
		desc.pVtx = vtx.data();
		desc.vtxSize = sizeof(Vertex);
		desc.vtxCount = static_cast<UINT>(vtx.size());
		desc.pIdx = idx.data();
		desc.idxSize = sizeof(unsigned int);
		desc.idxCount = static_cast<UINT>(idx.size());
		desc.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		mesh.mesh = std::make_shared<MeshBuffer>(desc);
		m_meshes.push_back(mesh);
	}

	CreateConstantBufferWrite(
		GetDevice(),
		sizeof(CBBoneCombMatrix),
		&m_BoneCombMtxCBuffer);
	assert(m_BoneCombMtxCBuffer);

	std::string dir = file;
	dir = dir.substr(0, dir.find_last_of('/') + 1);
	aiColor3D color(0.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT4 diffuse(1.0f, 1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 ambient(0.3f, 0.3f, 0.3f, 1.0f);
	for (unsigned int i = 0; i < m_pScene->mNumMaterials; ++i)
	{
		Material material = {};
		float shininess;
		material.diffuse = m_pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS ?
			DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f) : diffuse;
		material.ambient = m_pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS ?
			DirectX::XMFLOAT4(color.r, color.g, color.b, 1.0f) : ambient;
		shininess = m_pScene->mMaterials[i]->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS ? shininess : 0.0f;
		material.specular = m_pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS ?
			DirectX::XMFLOAT4(color.r, color.g, color.b, shininess) : DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, shininess);

		aiString path;
		if (m_pScene->mMaterials[i]->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS) {
			DebugLog::log(DebugLog::INFO_LOG, "Try to load texture: ", path.C_Str());
			HRESULT hr;
			if (strstr(path.C_Str(), ".psd"))
			{
				DebugLog::log(DebugLog::ERROR_LOG, "モデルのテクスチャにpsdファイルが指定されています。psd読み込みには非対応。");
				Error("モデルのテクスチャにpsdファイルが指定されています。psd読み込みには非対応。");
			}
			material.texture = std::make_shared<Texture>();
			hr = material.texture->Create(path.C_Str());
			if (FAILED(hr)) {
				std::string file = dir;
				file += path.C_Str();
				hr = material.texture->Create(file.c_str());
			}
			if (FAILED(hr)) {
				std::string file = path.C_Str();
				if (size_t idx = file.find_last_of('\\'); idx != std::string::npos)
				{
					file = file.substr(idx + 1);
					file = dir + file;
					hr = material.texture->Create(file.c_str());
				}
				if (FAILED(hr)) {
					if (size_t idx = file.find_last_of('/'); idx != std::string::npos)
					{
						file = file.substr(idx + 1);
						file = dir + file;
						hr = material.texture->Create(file.c_str());
					}
				}
			}
			if (FAILED(hr)) {
				Error(path.C_Str());
				material.texture = nullptr;
				DebugLog::log(DebugLog::ERROR_LOG, "Failed to create texture: ", path.C_Str());
			}
		}
		else {
			material.texture = nullptr;
		}
		m_materials.push_back(material);
	}

	DebugLog::log(DebugLog::INFO_LOG, "Loaded Material Count = ", (int)m_materials.size());
	DebugLog::log(DebugLog::INFO_LOG, "モデル読み込み完了");
	return true;
}


void Model::LoadAnimation(const char* FileName, const char* Name, bool flip)
{
	int flag = 0;
	if (flip) flag |= aiProcess_ConvertToLeftHanded;

	const aiScene* pAnimScene = aiImportFile(FileName, flag);

	if (!pAnimScene)
	{
		DebugLog::log(DebugLog::ERROR_LOG, "Failed to load animation file, check the path: ", FileName);
		return;
	}

	if (!pAnimScene->HasAnimations())
	{
		DebugLog::log(DebugLog::WARNING_LOG, "Animation file loaded, but it contains no animations: ", FileName);
		aiReleaseImport(pAnimScene);
		return;
	}

	m_Animation[Name] = pAnimScene;
	DebugLog::log(DebugLog::INFO_LOG, "Animation loaded successfully: ", Name);
}

void Model::Draw(int texSlot)
{
	m_pVS->Bind();
	m_pPS->Bind();
	auto it = m_meshes.begin();
	GetContext()->VSSetConstantBuffers(5, 1, &m_BoneCombMtxCBuffer);
	while (it != m_meshes.end())
	{
		if (texSlot >= 0)
		{
			if (m_materials.empty() || m_materials[it->materialID].texture.get() == nullptr)
			{
				DebugLog::log(DebugLog::WARNING_LOG, "Texture is NULL when drawing mesh.");
			}
			m_pPS->SetTexture(texSlot, m_materials[it->materialID].texture.get());
		}
		it->mesh->Draw();
		++it;
	}
}

void Model::RemakeVertex(int vtxSize, const std::function<void(RemakeInfo& data)>& func)
{
	auto it = m_meshes.begin();
	while (it != m_meshes.end())
	{
		MeshBuffer::Description desc = it->mesh->GetDesc();

		char* newVtx = new char[vtxSize * desc.vtxCount];
		RemakeInfo data = {};
		data.vtxNum = desc.vtxCount;
		data.dest = newVtx;
		data.source = desc.pVtx;
		data.idxNum = desc.idxCount;
		data.idx = desc.pIdx;
		func(data);

		desc.pVtx = newVtx;
		desc.vtxSize = vtxSize;
		it->mesh = std::make_shared<MeshBuffer>(desc);

		delete[] newVtx;
		++it;
	}
}

void Model::MakeDefaultShader()
{
	const char* vsCode = R"EOT(
struct VS_IN {
	float3 pos : POSITION0;
	float3 normal : NORMAL0;
	float2 uv : TEXCOORD0;
};
struct VS_OUT {
	float4 pos : SV_POSITION;
};
VS_OUT main(VS_IN vin) {
	VS_OUT vout;
	vout.pos = float4(vin.pos, 1.0f);
	vout.pos.z *= 0.1f;
	return vout;
})EOT";
	const char* psCode = R"EOT(
struct PS_IN {
	float4 pos : SV_POSITION;
};
float4 main(PS_IN pin) : SV_TARGET
{
	return float4(1.0f, 0.0f, 1.0f, 1.0f);
})EOT";
	m_defVS = std::make_shared<VertexShader>();
	m_defVS->Compile(vsCode);
	m_defPS = std::make_shared<PixelShader>();
	m_defPS->Compile(psCode);
}

void Model::CreateBone(const aiNode* node)
{
	BONE bone;

	m_Bone[node->mName.C_Str()] = bone;
	DebugLog::log(DebugLog::INFO_LOG, "BoneName = ", node->mName.C_Str());

	for (unsigned int n = 0; n < node->mNumChildren; n++)
	{
		CreateBone(node->mChildren[n]);
	}

}

// 変更: この関数はブレンド関数のラッパー（簡易版）として機能するように変更
void Model::UpdateAnimation(const char* AnimationName, int Frame)
{
	UpdateWithBlend(AnimationName, Frame, AnimationName, Frame, 1.0f);
}

// 追加: アニメーションをブレンドして更新する
void Model::UpdateWithBlend(const char* newAnimName, int newFrame, const char* oldAnimName, int oldFrame, float blendFactor)
{
	// ブレンド率が1.0以上なら、新しいアニメーションだけを計算すればよい
	if (blendFactor >= 1.0f)
	{
		CalculateAnimationPose(newAnimName, newFrame, m_bonecombmtxcontainer);
	}
	else
	{
		// 新旧両方のアニメーション姿勢を計算
		std::vector<Matrix> newPose, oldPose;
		CalculateAnimationPose(newAnimName, newFrame, newPose);
		CalculateAnimationPose(oldAnimName, oldFrame, oldPose);

		if (newPose.empty() || oldPose.empty() || newPose.size() != oldPose.size())
		{
			// どちらかの計算に失敗したら、新しいポーズをそのまま使う
			m_bonecombmtxcontainer = newPose;
		}
		else
		{
			// 両方のポーズを補間（ブレンド）する
			size_t boneCount = newPose.size();
			m_bonecombmtxcontainer.resize(boneCount);

			for (size_t i = 0; i < boneCount; ++i)
			{
				Vector3 oldScale, oldTrans, newScale, newTrans;
				Quaternion oldRot, newRot;

				oldPose[i].Decompose(oldScale, oldRot, oldTrans);
				newPose[i].Decompose(newScale, newRot, newTrans);

				// 各要素を線形補間（回転は球面線形補間）
				Vector3 finalScale = Vector3::Lerp(oldScale, newScale, blendFactor);
				Vector3 finalTrans = Vector3::Lerp(oldTrans, newTrans, blendFactor);
				Quaternion finalRot = Quaternion::Slerp(oldRot, newRot, blendFactor);

				// 補間結果から最終的な行列を再合成
				m_bonecombmtxcontainer[i] = Matrix::CreateScale(finalScale) *
					Matrix::CreateFromQuaternion(finalRot) *
					Matrix::CreateTranslation(finalTrans);
			}
		}
	}

	// 転置して定数バッファに書き込む
	for (auto& bcmtx : m_bonecombmtxcontainer)
	{
		bcmtx.Transpose();
	}
	BoneUpdate();
}

// 追加: 特定のアニメーション・フレームのボーン姿勢を計算する
void Model::CalculateAnimationPose(const char* animName, int frame, std::vector<Matrix>& outPose)
{
	// 1. アニメーションデータを特定する
	aiAnimation* animation = nullptr;
	if (m_Animation.count(animName) > 0 && m_Animation.at(animName) != nullptr)
	{
		if (m_Animation.at(animName)->HasAnimations())
			animation = m_Animation.at(animName)->mAnimations[0];
	}
	else if (m_pScene && m_pScene->HasAnimations())
	{
		animation = m_pScene->mAnimations[0];
	}
	if (!animation) return;

	// 2. 各ボーンのローカルなアニメーション行列を計算する
	std::vector<Matrix> animMatrices(m_Bone.size());
	for (auto& boneData : m_Bone)
	{
		animMatrices[boneData.second.idx] = Matrix::Identity; // いったん単位行列で初期化
	}

	for (unsigned int c = 0; c < animation->mNumChannels; c++)
	{
		aiNodeAnim* nodeAnim = animation->mChannels[c];
		if (m_Bone.count(nodeAnim->mNodeName.C_Str()) == 0) continue;

		BONE* bone = &m_Bone[nodeAnim->mNodeName.C_Str()];

		int rotFrame = frame % nodeAnim->mNumRotationKeys;
		aiQuaternion rot = nodeAnim->mRotationKeys[rotFrame].mValue;

		int posFrame = frame % nodeAnim->mNumPositionKeys;
		aiVector3D pos = nodeAnim->mPositionKeys[posFrame].mValue;

		int scaleFrame = frame % nodeAnim->mNumScalingKeys;
		aiVector3D scale = nodeAnim->mScalingKeys[scaleFrame].mValue;

		Matrix scaleMat = Matrix::CreateScale(scale.x, scale.y, scale.z);
		Matrix rotMat = Matrix::CreateFromQuaternion({ rot.x, rot.y, rot.z, rot.w });
		Matrix transMat = Matrix::CreateTranslation(pos.x, pos.y, pos.z);

		animMatrices[bone->idx] = scaleMat * rotMat * transMat;
	}

	// 3. 再帰的に親子関係を辿って最終的な行列を計算する
	outPose.resize(m_Bone.size());
	Matrix rootMatrix = Matrix::Identity;
	CalculateBoneMatrixRecursive(m_pScene->mRootNode, rootMatrix, animMatrices, outPose);
}

// 追加: CalculateAnimationPose内で使用する再帰関数
void Model::CalculateBoneMatrixRecursive(const aiNode* node, const Matrix& parentMatrix, const std::vector<Matrix>& animMatrices, std::vector<Matrix>& outPose)
{
	std::string nodeName = node->mName.C_Str();
	Matrix nodeTransform = aiMtxToDxMtx(node->mTransformation);

	if (m_Bone.count(nodeName) > 0)
	{
		int boneIdx = m_Bone[nodeName].idx;
		nodeTransform = animMatrices[boneIdx]; // アニメーションデータがあればそちらを優先
	}

	Matrix globalTransform = nodeTransform * parentMatrix;

	if (m_Bone.count(nodeName) > 0)
	{
		int boneIdx = m_Bone[nodeName].idx;
		outPose[boneIdx] = m_Bone[nodeName].OffsetMatrix * globalTransform;
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		CalculateBoneMatrixRecursive(node->mChildren[i], globalTransform, animMatrices, outPose);
	}
}


void Model::UpdateBoneMatrix(const aiNode* node, const Matrix& matrix)
{
	if (node->mName.length <= 0)
	{
		DebugLog::log(DebugLog::ERROR_LOG, "ノード(ボーン)名が取得できていないorない");
		Error("ノード(ボーン)名が取得できていないorない");
		return;
	}
	BONE* bone = &m_Bone[node->mName.C_Str()];
	Matrix bonecombinationmtx = bone->OffsetMatrix * bone->AnimationMatrix * matrix;
	bone->Matrix = bonecombinationmtx;
	Matrix mybonemtx = bone->AnimationMatrix * matrix;

	for (unsigned int n = 0; n < node->mNumChildren; n++)
	{
		UpdateBoneMatrix(node->mChildren[n], mybonemtx);
	}
}

bool  Model::CreateConstantBufferWrite(
	ID3D11Device* device,
	unsigned int bytesize,
	ID3D11Buffer** pConstantBuffer
) {

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = bytesize;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = device->CreateBuffer(&bd, nullptr, pConstantBuffer);
	if (FAILED(hr)) {
		MessageBox(nullptr, "CreateBuffer(constant buffer) error", "Error", MB_OK);
		return false;
	}

	return true;
}

void Model::BoneUpdate()
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	CBBoneCombMatrix* pData = nullptr;

	HRESULT hr = GetContext()->Map(
		m_BoneCombMtxCBuffer,
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&MappedResource);

	if (SUCCEEDED(hr)) {
		pData = (CBBoneCombMatrix*)MappedResource.pData;
		if (!m_bonecombmtxcontainer.empty())
		{
			memcpy(pData->BoneCombMtx,
				m_bonecombmtxcontainer.data(),
				sizeof(Matrix) * m_bonecombmtxcontainer.size());
		}
		GetContext()->Unmap(m_BoneCombMtxCBuffer, 0);
	}
}

std::vector<Model::BONE> Model::GetBoneInfo(const aiMesh* mesh) {

	std::vector<BONE> bones;
	for (unsigned int bidx = 0; bidx < mesh->mNumBones; bidx++) {
		BONE bone{};
		bone.Bonename = std::string(mesh->mBones[bidx]->mName.C_Str());
		if (mesh->mBones[bidx]->mNode != nullptr)
		{
			bone.Meshname = std::string(mesh->mBones[bidx]->mNode->mName.C_Str());
		}
		else {
			DebugLog::log(DebugLog::WARNING_LOG, "ノード情報がNULL");
		}
		if (mesh->mBones[bidx]->mArmature != nullptr)
		{
			bone.Armaturename = std::string(mesh->mBones[bidx]->mArmature->mName.C_Str());
		}
		else {
			DebugLog::log(DebugLog::WARNING_LOG, "アーマチュア情報がNULL");
		}
		bone.OffsetMatrix = aiMtxToDxMtx(mesh->mBones[bidx]->mOffsetMatrix);
		bone.weights.clear();
		for (unsigned int widx = 0; widx < mesh->mBones[bidx]->mNumWeights; widx++) {

			WEIGHT w;
			w.meshname = bone.Meshname;
			w.bonename = bone.Bonename;
			w.weight = mesh->mBones[bidx]->mWeights[widx].mWeight;
			w.vertexindex = mesh->mBones[bidx]->mWeights[widx].mVertexId;
			bone.weights.emplace_back(w);
		}
		bones.emplace_back(bone);
		m_Bone[mesh->mBones[bidx]->mName.C_Str()].OffsetMatrix = aiMtxToDxMtx(mesh->mBones[bidx]->mOffsetMatrix);
	}
	return bones;
}


SimpleMath::Matrix Model::GetScaleBaseMatrix()
{
	return DirectX::XMMatrixScaling(m_scaleBase, m_scaleBase, m_scaleBase);
}

float Model::GetScaleBase()
{
	return m_scaleBase;
}

DirectX::SimpleMath::Matrix Model::aiMtxToDxMtx(const aiMatrix4x4& aimatrix) {

	DirectX::SimpleMath::Matrix dxmtx = {
	   aimatrix.a1,aimatrix.b1,aimatrix.c1,aimatrix.d1,
	   aimatrix.a2,aimatrix.b2,aimatrix.c2,aimatrix.d2,
	   aimatrix.a3,aimatrix.b3,aimatrix.c3,aimatrix.d3,
	   aimatrix.a4,aimatrix.b4,aimatrix.c4,aimatrix.d4
	};
	return dxmtx;
}