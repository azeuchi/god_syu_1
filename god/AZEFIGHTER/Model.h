#ifndef __MODEL_H__
#define __MODEL_H__

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string_view>
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

	// const char* や string_view のまま検索できるようにするハッシュ。
	// これが無いと、検索のたびにキーの std::string が作られてメモリ確保が起きる
	struct TransparentStringHash
	{
		using is_transparent = void;
		size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
	};
	using BoneMap = std::unordered_map<std::string, BONE, TransparentStringHash, std::equal_to<>>;
	using AnimMap = std::unordered_map<std::string, std::shared_ptr<const aiScene>, TransparentStringHash, std::equal_to<>>;

	struct CBBoneCombMatrix {
		XMFLOAT4X4 BoneCombMtx[400];
	};

private:
	// モデルデータのキャッシュ用構造体
	struct ModelCache {
		std::shared_ptr<Assimp::Importer> importer;
		const aiScene* scene;
		Meshes meshes;
		Materials materials;
		BoneMap bones;
	};

	// アニメーションデータのキャッシュ用構造体
	struct AnimCache {
		std::shared_ptr<const aiScene> scene;
	};

	// キャッシュの実体 (全Modelインスタンスで共有)
	static std::unordered_map<std::string, ModelCache> s_modelCache;
	static std::unordered_map<std::string, AnimCache> s_animCache;

public:
	Model();
	~Model();
	void SetVertexShader(Shader* vs);
	void SetPixelShader(Shader* ps);
	const Mesh* GetMesh(unsigned int index) const;
	unsigned int GetMeshNum() const;
	//アニメーションの総フレーム数を取得
	int GetAnimationTotalFrame(const char* animName);
public:
	bool Load(const char* file, float scaleBase = 1.0f, bool flip = false, bool simple = false);
	void LoadAnimation(const char* FileName, const char* Name, bool flip);
	void Draw(int texSlot = 0);
	void SetTexture(const char* fileName);

	void UpdateAnimation(const char* AnimationName, float Frame);

	// アニメーションをブレンドするための関数
	void UpdateWithBlend(const char* newAnimName, float newFrame, const char* oldAnimName, float oldFrame, float blendFactor);

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

	// 特定のアニメーション・フレームのボーン姿勢（Global行列）を計算するヘルパー関数
	void CalculateAnimationPose(const char* animName, float frame, std::vector<Matrix>& outPose);

	// アニメーションデータから「ローカル行列（親ボーンに対する S*R*T）」のみを取得する関数
	// frame は小数で渡し、前後のキーの間を補間する
	void SampleAnimationKeys(const char* animName, float frame, std::vector<Matrix>& outLocalMatrices);

	//  ローカル行列の配列を受け取り、親子関係を計算してグローバル行列にする再帰関数
	void CalculateBoneMatrixRecursive(const aiNode* node, const Matrix& parentMatrix, const std::vector<Matrix>& animMatrices, std::vector<Matrix>& outPose);


private:
	// Assimp::Importerをshared_ptrに変更してキャッシュと共有可能にする
	std::shared_ptr<Assimp::Importer> m_importer;
	const aiScene* m_pScene = nullptr;
	static std::shared_ptr<VertexShader> m_defVS;
	static std::shared_ptr<PixelShader> m_defPS;
	std::vector<Matrix> m_bonecombmtxcontainer;

	// 姿勢計算の作業用バッファ。毎フレーム確保し直さずに使い回す
	std::vector<Matrix> m_poseNew;
	std::vector<Matrix> m_poseOld;
	std::vector<Matrix> m_poseBlended;

	BoneMap m_Bone;
	ID3D11Buffer* m_BoneCombMtxCBuffer = nullptr;

	// アニメーションシーンの管理もshared_ptrに変更
	AnimMap m_Animation;

private:
	Meshes m_meshes;
	Materials m_materials;
	VertexShader* m_pVS;
	PixelShader* m_pPS;
	float m_scaleBase = 1.0f;
	char* filepath;
};


#endif // __MODEL_H__