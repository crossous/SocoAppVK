RWStructuredBuffer<int> correct : register(u0, space0);
RWStructuredBuffer<int> incorrect : register(u0, space1);

struct PSInput
{
	float4 color : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}