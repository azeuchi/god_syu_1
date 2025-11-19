#ifndef __MODEL_H__
#define __MODEL_H__

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "Shader.h"
#include "MeshBuffer.h"
#include "Texture.h"

#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/matrix4x4.h"

#include "DirectXTex/SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

class Model
{

public:
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		int		boneIndex[4];
		float	boneWeight[4];
		int		boneCount = 0;
	};
	struct Material
	{
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 specular;
		std::shared_ptr<Texture> texture;
	};
	using Materials = std::vector<Material>;
	struct Mesh
	{
		std::shared_ptr<MeshBuffer> mesh;
		unsigned int materialID;
	};
	using Meshes = std::vector<Mesh>;

	struct RemakeInfo
	{
		UINT vtxNum;
		void* dest;
		const void* source;
		UINT idxNum;
		const void* idx;
	};

	struct WEIGHT {
		std::string bonename;
		std::string meshname;
		float weight;
		int	vertexindex;
	};

	struct BONE
	{
		std::string Bonename;
		std::string Meshname;
		std::string Armaturename;
		SimpleMath::Matrix Matrix;
		SimpleMath::Matrix AnimationMatrix;
		SimpleMath::Matrix OffsetMatrix;
		int			idx;
		std::vector<WEIGHT> weights;
	};

	struct CBBoneCombMatrix {
		XMFLOAT4X4 BoneCombMtx[400];
	};
public:
	Model();
	~Model();
	void SetVertexShader(Shader* vs);
	void SetPixelShader(Shader* ps);
	const Mesh* GetMesh(unsigned int index) const;
	unsigned int GetMeshNum() const;

public:
	bool Load(const char* file, float scaleBase = 1.0f, bool flip = false, bool simple = false);
	void LoadAnimation(const char* FileName, const char* Name, bool flip);
	void Draw(int texSlot = 0);

	void UpdateAnimation(const char* AnimationName, int Frame);

	// 追加: アニメーションをブレンドするための関数
	void UpdateWithBlend(const char* newAnimName, int newFrame, const char* oldAnimName, int oldFrame, float blendFactor);

	void UpdateBoneMatrix(const aiNode* node, const Matrix& matrix);

	void RemakeVertex(int vtxSize, const std::function<void(RemakeInfo& data)>& func);
	void CreateBone(const aiNode* node);
	void BoneUpdate();
	SimpleMath::Matrix GetScaleBaseMatrix();
	float GetScaleBase();
private:
	void MakeDefaultShader();
	bool CreateConstantBufferWrite(
		ID3D11Device* device,
		unsigned int bytesize,
		ID3D11Buffer** pConstantBuffer
	);
	std::vector<BONE> GetBoneInfo(const aiMesh* mesh);
	DirectX::SimpleMath::Matrix aiMtxToDxMtx(const aiMatrix4x4& aimatrix);

	// 追加: 特定のアニメーション・フレームのボーン姿勢を計算するヘルパー関数
	void CalculateAnimationPose(const char* animName, int frame, std::vector<Matrix>& outPose);

	// 追加: アニメーションデータからローカル行列（S*R*T）を取得する関数
	void SampleAnimationKeys(const char* animName, int frame, std::vector<Matrix>& outLocalMatrices);

	// 追加: ↑の関数内で使う再帰関数
	void CalculateBoneMatrixRecursive(const aiNode* node, const Matrix& parentMatrix, const std::vector<Matrix>& animMatrices, std::vector<Matrix>& outPose);


private:
	Assimp::Importer* importer = nullptr;
	const aiScene* m_pScene = nullptr;
	static std::shared_ptr<VertexShader> m_defVS;
	static std::shared_ptr<PixelShader> m_defPS;
	std::vector<Matrix> m_bonecombmtxcontainer;

	std::unordered_map<std::string, BONE> m_Bone;
	ID3D11Buffer* m_BoneCombMtxCBuffer = nullptr;
	std::unordered_map<std::string, const aiScene*> m_Animation;

private:
	Meshes m_meshes;
	Materials m_materials;
	VertexShader* m_pVS;
	PixelShader* m_pPS;
	float m_scaleBase = 1.0f;
	char* filepath;
};


#endif // __MODEL_H__