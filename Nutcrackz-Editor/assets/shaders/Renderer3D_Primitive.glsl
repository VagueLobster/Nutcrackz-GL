// ==========================
// * Nutcrackz 2D *
// Renderer3D Primitives Shader
// ==========================

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;
layout (location = 3) in vec2 a_TexCoord;
layout (location = 4) in vec2 a_TilingFactor;
layout (location = 5) in int a_TexIndex;

layout (location = 6) in vec3 a_MatSpecular;
layout (location = 7) in float a_MatShininess;

layout (location = 8) in vec3 a_LightPosition;
layout (location = 9) in vec3 a_LightAmbient;
layout (location = 10) in vec3 a_LightDiffuse;
layout (location = 11) in vec3 a_LightSpecular;

layout (location = 12) in vec3 a_ViewPos;

layout (location = 13) in int a_SpotLightExists;
layout (location = 14) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec4 Color;
	vec2 TexCoord;
	vec2 TilingFactor;
};

struct MiscOutput
{
	vec3 ViewPos;
};

struct MaterialOutput
{
	vec3 Specular;
};

struct LightOutput
{
	vec3 Position;
	vec3 Ambient;
	vec3 Diffuse;
	vec3 Specular;
};

layout (location = 0) out VertexOutput Output;
layout (location = 6) out MaterialOutput Material;
layout (location = 7) out MiscOutput Misc;
layout (location = 8) out LightOutput Light;
layout (location = 13) out flat int v_TexIndex;
layout (location = 14) out flat float v_Shininess;
layout (location = 15) out flat int v_SpotLightExists;
layout (location = 16) out flat int v_EntityID;

void main()
{
	Output.Position = a_Position;
	Output.Normal = a_Normal;
	Output.Color = a_Color;
	Output.TexCoord = a_TexCoord;
	Output.TilingFactor = a_TilingFactor;

	Material.Specular = a_MatSpecular;

	Light.Position = a_LightPosition;
	Light.Ambient = a_LightAmbient;
	Light.Diffuse = a_LightDiffuse;
	Light.Specular = a_LightSpecular;

	Misc.ViewPos = a_ViewPos;

	v_TexIndex = a_TexIndex;
	v_Shininess = a_MatShininess;

	v_SpotLightExists = a_SpotLightExists;
	v_EntityID = a_EntityID;

	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec3 Position;
	vec3 Normal;
	vec4 Color;
	vec2 TexCoord;
	vec2 TilingFactor;
};

struct MiscOutput
{
	vec3 ViewPos;
};

struct MaterialOutput
{
	vec3 Specular;
};

struct LightOutput
{
	vec3 Position;
	vec3 Ambient;
	vec3 Diffuse;
	vec3 Specular;
};

layout (location = 0) in VertexOutput Input;
layout (location = 6) in MaterialOutput Material;
layout (location = 7) in MiscOutput Misc;
layout (location = 8) in LightOutput Light;
layout (location = 13) in flat int v_TexIndex;
layout (location = 14) in flat float v_Shininess;
layout (location = 15) in flat int v_SpotLightExists;
layout (location = 16) in flat int v_EntityID;

layout(binding = 0) uniform sampler2D u_Textures[32];

void main()
{
	vec4 texColor = Input.Color;

	switch(v_TexIndex)
	{
		case  0: texColor *= texture(u_Textures[ 0], Input.TexCoord * Input.TilingFactor); break;
		case  1: texColor *= texture(u_Textures[ 1], Input.TexCoord * Input.TilingFactor); break;
		case  2: texColor *= texture(u_Textures[ 2], Input.TexCoord * Input.TilingFactor); break;
		case  3: texColor *= texture(u_Textures[ 3], Input.TexCoord * Input.TilingFactor); break;
		case  4: texColor *= texture(u_Textures[ 4], Input.TexCoord * Input.TilingFactor); break;
		case  5: texColor *= texture(u_Textures[ 5], Input.TexCoord * Input.TilingFactor); break;
		case  6: texColor *= texture(u_Textures[ 6], Input.TexCoord * Input.TilingFactor); break;
		case  7: texColor *= texture(u_Textures[ 7], Input.TexCoord * Input.TilingFactor); break;
		case  8: texColor *= texture(u_Textures[ 8], Input.TexCoord * Input.TilingFactor); break;
		case  9: texColor *= texture(u_Textures[ 9], Input.TexCoord * Input.TilingFactor); break;
		case 10: texColor *= texture(u_Textures[10], Input.TexCoord * Input.TilingFactor); break;
		case 11: texColor *= texture(u_Textures[11], Input.TexCoord * Input.TilingFactor); break;
		case 12: texColor *= texture(u_Textures[12], Input.TexCoord * Input.TilingFactor); break;
		case 13: texColor *= texture(u_Textures[13], Input.TexCoord * Input.TilingFactor); break;
		case 14: texColor *= texture(u_Textures[14], Input.TexCoord * Input.TilingFactor); break;
		case 15: texColor *= texture(u_Textures[15], Input.TexCoord * Input.TilingFactor); break;
		case 16: texColor *= texture(u_Textures[16], Input.TexCoord * Input.TilingFactor); break;
		case 17: texColor *= texture(u_Textures[17], Input.TexCoord * Input.TilingFactor); break;
		case 18: texColor *= texture(u_Textures[18], Input.TexCoord * Input.TilingFactor); break;
		case 19: texColor *= texture(u_Textures[19], Input.TexCoord * Input.TilingFactor); break;
		case 20: texColor *= texture(u_Textures[20], Input.TexCoord * Input.TilingFactor); break;
		case 21: texColor *= texture(u_Textures[21], Input.TexCoord * Input.TilingFactor); break;
		case 22: texColor *= texture(u_Textures[22], Input.TexCoord * Input.TilingFactor); break;
		case 23: texColor *= texture(u_Textures[23], Input.TexCoord * Input.TilingFactor); break;
		case 24: texColor *= texture(u_Textures[24], Input.TexCoord * Input.TilingFactor); break;
		case 25: texColor *= texture(u_Textures[25], Input.TexCoord * Input.TilingFactor); break;
		case 26: texColor *= texture(u_Textures[26], Input.TexCoord * Input.TilingFactor); break;
		case 27: texColor *= texture(u_Textures[27], Input.TexCoord * Input.TilingFactor); break;
		case 28: texColor *= texture(u_Textures[28], Input.TexCoord * Input.TilingFactor); break;
		case 29: texColor *= texture(u_Textures[29], Input.TexCoord * Input.TilingFactor); break;
		case 30: texColor *= texture(u_Textures[30], Input.TexCoord * Input.TilingFactor); break;
		case 31: texColor *= texture(u_Textures[31], Input.TexCoord * Input.TilingFactor); break;
	}

	if (v_SpotLightExists > 0)
	{
		// Ambient
		vec3 ambient = Light.Ambient * texColor.rgb;
  	
		// Diffuse 
		vec3 norm = normalize(Input.Normal);
		vec3 lightDir = normalize(Light.Position - Input.Position);
		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = Light.Diffuse * diff * texColor.rgb;  
    
		// Specular
		vec3 viewDir = normalize(Misc.ViewPos - Input.Position);
		vec3 reflectDir = reflect(-lightDir, norm);  
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), v_Shininess);
		vec3 specular = Light.Specular * (spec * Material.Specular);  
        
		vec4 result = vec4(ambient + diffuse + specular, texColor.a);

		if (result.a == 0.0)
			discard;

		o_Color = result;
	}
	else
	{
		if (texColor.a == 0.0)
				discard;

		o_Color = texColor;
	}

	o_EntityID = v_EntityID;
}