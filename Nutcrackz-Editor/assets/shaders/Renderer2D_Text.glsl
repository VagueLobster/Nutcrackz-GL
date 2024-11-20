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

float screenPxRange(int index)
{
	const float pxRange = 2.0; // set to distance field's pixel range
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_FontAtlases[index], 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(Input.TexCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    float opacity = 0.0;	
	switch(v_TexIndex)
	{
		case  0:
		{
			vec3 msd = texture(u_FontAtlases[0], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(0) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  1:
		{
			vec3 msd = texture(u_FontAtlases[1], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(1) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  2:
		{
			vec3 msd = texture(u_FontAtlases[2], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(2) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  3:
		{
			vec3 msd = texture(u_FontAtlases[3], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(3) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  4:
		{
			vec3 msd = texture(u_FontAtlases[4], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(4) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  5:
		{
			vec3 msd = texture(u_FontAtlases[5], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(5) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  6:
		{
			vec3 msd = texture(u_FontAtlases[6], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(6) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  7:
		{
			vec3 msd = texture(u_FontAtlases[7], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(7) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  8:
		{
			vec3 msd = texture(u_FontAtlases[8], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(8) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  9:
		{
			vec3 msd = texture(u_FontAtlases[9], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(9) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  10:
		{
			vec3 msd = texture(u_FontAtlases[10], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(10) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  11:
		{
			vec3 msd = texture(u_FontAtlases[11], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(11) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  12:
		{
			vec3 msd = texture(u_FontAtlases[12], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(12) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  13:
		{
			vec3 msd = texture(u_FontAtlases[13], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(13) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  14:
		{
			vec3 msd = texture(u_FontAtlases[14], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(14) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  15:
		{
			vec3 msd = texture(u_FontAtlases[15], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(15) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  16:
		{
			vec3 msd = texture(u_FontAtlases[16], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(16) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  17:
		{
			vec3 msd = texture(u_FontAtlases[17], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(17) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  18:
		{
			vec3 msd = texture(u_FontAtlases[18], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(18) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  19:
		{
			vec3 msd = texture(u_FontAtlases[19], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(19) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  20:
		{
			vec3 msd = texture(u_FontAtlases[20], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(20) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  21:
		{
			vec3 msd = texture(u_FontAtlases[21], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(21) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  22:
		{
			vec3 msd = texture(u_FontAtlases[22], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(22) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  23:
		{
			vec3 msd = texture(u_FontAtlases[23], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(23) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  24:
		{
			vec3 msd = texture(u_FontAtlases[24], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(24) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  25:
		{
			vec3 msd = texture(u_FontAtlases[25], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(25) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  26:
		{
			vec3 msd = texture(u_FontAtlases[26], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(26) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  27:
		{
			vec3 msd = texture(u_FontAtlases[27], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(27) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  28:
		{
			vec3 msd = texture(u_FontAtlases[28], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(28) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  29:
		{
			vec3 msd = texture(u_FontAtlases[29], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(29) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  30:
		{
			vec3 msd = texture(u_FontAtlases[30], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(30) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
		case  31:
		{
			vec3 msd = texture(u_FontAtlases[31], Input.TexCoord).rgb;
			float sd = median(msd.r, msd.g, msd.b);
			float screenPxDistance = screenPxRange(31) * (sd - 0.5);
			opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
			break;
		}
	}

	if (opacity == 0.0)
		discard;
	
	vec4 bgColor = vec4(0.0, 0.0, 0.0, 0.0001);
    o_Color = mix(bgColor, Input.Color, opacity);
	//o_Color = mix(Input.BgColor, Input.Color, opacity);
	if (o_Color.a == 0.0)
		discard;

	o_EntityID = v_EntityID;
}