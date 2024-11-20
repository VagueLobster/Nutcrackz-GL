// MSDF text shader

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in vec4 a_BgColor;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in int a_TexIndex;
layout (location = 5) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec4 Color;
	vec4 BgColor;
	vec2 TexCoord;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat int v_TexIndex;
layout (location = 4) out flat int v_EntityID;

void main()
{
	Output.Color = a_Color;
	Output.BgColor = a_BgColor;
	Output.TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec4 BgColor;
	vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat int v_TexIndex;
layout (location = 4) in flat int v_EntityID;

layout (binding = 0) uniform sampler2D u_FontAtlases[32];
//layout (binding = 0) uniform sampler2D u_FontAtlas;

float screenPxRange(int index)
{
	const float pxRange = 2.0; // set to distance field's pixel range
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(u_FontAtlases[index], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(Input.TexCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float getOpacity(int index)
{
	vec3 msd = texture(u_FontAtlases[index], Input.TexCoord).rgb;
	float sd = median(msd.r, msd.g, msd.b);
	float screenPxDistance = screenPxRange(index)*(sd - 0.5);
	return clamp(screenPxDistance + 0.5, 0.0, 1.0);
}

void main()
{
    float opacity = 0.0;	
	switch(v_TexIndex)
	{
		case  0: opacity = getOpacity( 0); break;
		case  1: opacity = getOpacity( 1); break;
		case  2: opacity = getOpacity( 2); break;
		case  3: opacity = getOpacity( 3); break;
		case  4: opacity = getOpacity( 4); break;
		case  5: opacity = getOpacity( 5); break;
		case  6: opacity = getOpacity( 6); break;
		case  7: opacity = getOpacity( 7); break;
		case  8: opacity = getOpacity( 8); break;
		case  9: opacity = getOpacity( 9); break;
		case 10: opacity = getOpacity(10); break;
		case 11: opacity = getOpacity(11); break;
		case 12: opacity = getOpacity(12); break;
		case 13: opacity = getOpacity(13); break;
		case 14: opacity = getOpacity(14); break;
		case 15: opacity = getOpacity(15); break;
		case 16: opacity = getOpacity(16); break;
		case 17: opacity = getOpacity(17); break;
		case 18: opacity = getOpacity(18); break;
		case 19: opacity = getOpacity(19); break;
		case 20: opacity = getOpacity(20); break;
		case 21: opacity = getOpacity(21); break;
		case 22: opacity = getOpacity(22); break;
		case 23: opacity = getOpacity(23); break;
		case 24: opacity = getOpacity(24); break;
		case 25: opacity = getOpacity(25); break;
		case 26: opacity = getOpacity(26); break;
		case 27: opacity = getOpacity(27); break;
		case 28: opacity = getOpacity(28); break;
		case 29: opacity = getOpacity(29); break;
		case 30: opacity = getOpacity(30); break;
		case 31: opacity = getOpacity(31); break;
	}

	if (opacity == 0.0)
		discard;
	
	vec4 bgColor = vec4(0.0);
    o_Color = mix(bgColor, Input.Color, opacity);
	//o_Color = mix(Input.BgColor, Input.Color, getOpacity());
	if (o_Color.a == 0.0)
		discard;

	o_EntityID = v_EntityID;
}