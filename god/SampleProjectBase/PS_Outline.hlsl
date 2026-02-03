// 頂点シェーダーから受け取るためのデータを定義
struct PS_IN
{
    float4 pos : SV_POSITION0;
    float2 uv : TEXCOORD0;
};

float4 main(PS_IN pin) : SV_TARGET
{
	// アウトラインの色（黒）
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}