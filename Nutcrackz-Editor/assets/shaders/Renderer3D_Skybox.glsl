// ==========================
// * Nutcrackz 3D *
// Renderer3D Skybox Shader
// ==========================

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Color;
layout (location = 2) in mat4 a_View;
layout (location = 6) in mat4 a_Proj;
layout (location = 10) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

/*layout(std140, binding = 1) uniform ViewProj
{
	mat4 u_View;
	mat4 u_Projection;
};*/

struct VertexOutput
{
	vec4 Color;
	vec3 TexCoord;
};

layout (location = 0) out VertexOutput Output;
layout (location = 2) out flat int v_EntityID;

void main()
{
	Output.Color = a_Color;
	Output.TexCoord = vec3(a_Position.x, -a_Position.y, -a_Position.z);

	v_EntityID = a_EntityID;

	//vec4 position = u_ViewProjection * vec4(a_Position, 1.0);
	//vec4 position = u_Projection * u_View * vec4(a_Position, 1.0);
	vec4 position = a_Proj * a_View * vec4(a_Position, 1.0);

	gl_Position = position.xyww;
	//gl_Position = position;
}

#type fragment
#version 450 core

layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec3 TexCoord;
};

layout (location = 0) in VertexOutput Input;
layout (location = 2) in flat int v_EntityID;

//layout (binding = 0) uniform samplerCube u_Skybox[32];
layout (binding = 0) uniform samplerCube u_Skybox;

void main()
{
	vec4 skyColor = Input.Color;// * texture(u_Skybox, Input.TexCoord);
	skyColor *= texture(u_Skybox, Input.TexCoord);

	o_Color = skyColor;

	o_EntityID = v_EntityID;
}