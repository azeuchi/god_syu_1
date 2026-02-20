struct VS_IN
{
    // SV_POSITIONÇÕì¸óÕÇ…ÇÕégÇ¶Ç»Ç¢ÇΩÇﬂ POSITION0 Ç…èCê≥
    float3 pos : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float4 screenPos : POSITION0;
};

cbuffer WVP : register(b0)
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
};

cbuffer Pram : register(b1)
{
    float2 offset;
    float2 size;
    float2 uvPos;
    float2 uvScale;
    float4 color;
};

cbuffer LightMat : register(b2)
{
    float4x4 Lworld;
    float4x4 Lview;
    float4x4 Lproj;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;
    vout.pos = float4(vin.pos, 1.0f);
    vout.pos.xy *= size;
    vout.pos.xy += offset;
    float4 pos = vout.pos;
     
    vout.pos = mul(vout.pos, world);
    vout.pos = mul(vout.pos, view);
    vout.pos = mul(vout.pos, proj);
    
    //  åıåπÇÃÉJÉÅÉâ
    pos = mul(pos, Lworld);
    pos = mul(pos, Lview);
    pos = mul(pos, Lproj);
    vout.screenPos = pos;
    
    vout.uv = vin.uv;
    vout.uv *= uvScale;
    vout.uv += uvPos;
    vout.color = color;
    return vout;
}