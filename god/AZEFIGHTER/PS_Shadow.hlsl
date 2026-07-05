struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float4 screenPos : POSITION0;
};

SamplerState samp : register(s0);
Texture2D depthTex : register(t0);


float4 main(PS_IN pin) : SV_TARGET
{
    float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    //頂点座標から画面上のUV座標を取得
    float2 screenUV = pin.screenPos.xy / pin.screenPos.w;
    screenUV = screenUV * 0.5f + 0.5f;
    screenUV.y = 1.0f - screenUV.y;
    
    //頂点座標から、カメラからの奥行きを取得
    float objDepth = pin.screenPos.z / pin.screenPos.w;
    
    //テクスチャに書き込まれている奥行き情報を取得
    float texDepth = depthTex.Sample(samp, screenUV).r;
    
    //範囲外は、遮るものがない（奥行きが一番奥）とする
    if (screenUV.x < 0.0f || screenUV.x > 1.0f ||
       screenUV.y < 0.0f || screenUV.y > 1.0f)
    {
        texDepth = 1.0f;
    }
    
    // 奥行きの情報を比較してシルエットを表示
    float shadow = step(objDepth, texDepth + 0.005f);
    
    // 影の色を濃いグレー(0.3)にして強調する
    color.rgb = lerp(float3(0.3f, 0.3f, 0.3f), float3(1.0f, 1.0f, 1.0f), shadow);
    
    return color;
}