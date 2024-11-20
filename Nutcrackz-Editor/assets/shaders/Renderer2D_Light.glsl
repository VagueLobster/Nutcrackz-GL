// ==========================
// * Nutcrackz 2D *
// Renderer2D Light Shader
// ==========================

#type vertex
#version 450 core

layout (location = 0) in vec3 a_WorldPosition;
layout (location = 1) in vec3 a_LocalPosition;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in float a_Intensity;
layout (location = 4) in float a_Fade;
layout (location = 5) in int a_EntityID;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 LocalPosition;
	vec4 Color;
	float Intensity;
	float Fade;
};

layout (location = 0) out VertexOutput Output;
layout (location = 5) out flat int v_EntityID;

void main()
{
	Output.WorldPosition = a_WorldPosition;
	Output.LocalPosition = a_LocalPosition;
	Output.Color = a_Color;
	Output.Intensity = a_Intensity;
	Output.Fade = a_Fade;
	
	v_EntityID = a_EntityID;
	gl_Position = u_ViewProjection * vec4(a_WorldPosition, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 LocalPosition;
	vec4 Color;
	float Intensity;
	float Fade;
};

layout (location = 0) in VertexOutput Input;
layout (location = 5) in flat int v_EntityID;

void main()
{
	float fade = Input.Fade;
	float intensity = Input.Intensity;

    float distance = 1.0 - length(Input.LocalPosition);
    float intensityPos = 1.0 / length(Input.LocalPosition);
    
	//float light = smoothstep(0.0, intensity + fade, 1.0 - distance);
	float light = smoothstep(0.0, intensity + fade, distance);

	if (light == 0.0)
		discard;

	o_Color = Input.Color * intensityPos;
	o_Color.a *= light;

	o_EntityID = v_EntityID;
}
