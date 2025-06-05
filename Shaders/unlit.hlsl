// [[vk::binding(0, 0)]]
cbuffer PerCamera : register(b0)
{
	float4x4 WorldToClipMatrix;
}

// [[vk::binding(1, 0)]]
cbuffer PerMaterial : register(b1, space2)
{
    float4 _MainTex_ST;
    float4 _Color;
}

// [[vk::binding(2, 0)]]
cbuffer PerObject : register(b2, space5)
{
	float4x4 ObjectToWorldMatrix;
	float4x4 WorldToObjectMatrix;
}

//[[vk::binding(0, 1)]]
Texture2D _MainTex : register(t0);
//[[vk::binding(0, 3)]]
SamplerState gsamLinearWrapAniso2[3] : register(s0);

struct Attributes
{
    float3 positionOS : POSITION;
    float3 normalOS : NORMAL;
    float3 tangentOS : TANGENT;
    float2 uv0 : TEXCOORD;
    float2 uv1 : TEXCOORD1;
	float3 color : COLOR;
};

struct Varyings
{
	float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD;
    float2 uv1 : TEXCOORD1;
	float3 color : COLOR;
};


Varyings vert(Attributes input)
{
	Varyings output = (Varyings)0;
    //output.positionCS = float4(input.positionOS, 1);

    //output.positionCS = mul(WorldToClipMatrix, float4(input.positionOS, 1));
	output.positionCS = mul(WorldToClipMatrix, mul(ObjectToWorldMatrix, float4(input.positionOS, 1)));
	output.color = input.color;
 //   output.uv = input.uv0 * _MainTex_ST.xy + _MainTex_ST.zw;
 //   output.uv1 = input.uv1;

	return output;
}

float4 frag(Varyings input) : SV_TARGET
{
    //return float4(input.color, 1);
    float4 color = _MainTex.Sample(gsamLinearWrapAniso2[0], input.uv);
    return float4(color.rgb * input.color * _Color.rgb + 0.5f, 1);
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   