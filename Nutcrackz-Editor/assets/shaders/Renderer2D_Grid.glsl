// ==========================
// * Nutcrackz 2D *
// Renderer2D Grid Shader
// ==========================

#type vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in int a_EntityID;
layout (location = 2) in float a_Near;
layout (location = 3) in float a_Far;
layout (location = 4) in mat4 a_View;
layout (location = 8) in mat4 a_Proj;
layout (location = 12) in vec3 a_CamPosition;
layout (location = 13) in float a_CamDistance;

struct VertexOutput
{
	vec3 v_NearPoint;
	vec3 v_FarPoint;
	mat4 v_View;
    mat4 v_Proj;
	vec3 v_CamPosition;
};

layout (location = 0) out VertexOutput Output;
layout (location = 11) out flat float v_Near;
layout (location = 12) out flat float v_Far;
layout (location = 13) out flat float v_CamDistance;
layout (location = 14) out flat int v_EntityID;

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection)
{
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main()
{
	vec3 p = a_Position;

    Output.v_NearPoint = UnprojectPoint(p.x, p.y, 0.0, a_View, a_Proj);
    Output.v_FarPoint = UnprojectPoint(p.x, p.y, 1.0, a_View, a_Proj);	
    Output.v_View = a_View;
    Output.v_Proj = a_Proj;
    Output.v_CamPosition = a_CamPosition;
    
	v_Near = a_Near;
    v_Far = a_Far;
    v_CamDistance = a_CamDistance;
	v_EntityID = a_EntityID;
    
	gl_Position = vec4(p, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec3 v_NearPoint;
	vec3 v_FarPoint;
	mat4 v_View;
    mat4 v_Proj;
	vec3 v_CamPosition;
};

layout (location = 0) in VertexOutput Input;
layout (location = 11) in flat float v_Near;
layout (location = 12) in flat float v_Far;
layout (location = 13) in flat float v_CamDistance;
layout (location = 14) in flat int v_EntityID;

vec4 grid(vec3 fragPos3D, float scale)
{
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord) * 1.0;
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);

    //vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
    vec4 color = vec4(0.15, 0.15, 0.15, 1.0 - min(line, 1.0));

    // z axis
    if(fragPos3D.x > -0.5 * minimumx && fragPos3D.x < 0.5 * minimumx)
        color.z = 1.0;
    
    // x axis
    if(fragPos3D.z > -0.5 * minimumz && fragPos3D.z < 0.5 * minimumz)
        color.x = 1.0;

    return color;
}

float computeDepth(vec3 pos)
{
    vec4 clip_space_pos = Input.v_Proj * Input.v_View * vec4(pos.xyz, 1.0);
    float returnValue = (clip_space_pos.z / clip_space_pos.w);

    //return (clip_space_pos.z / clip_space_pos.w); // Use this for Vulkan!!!
    return (returnValue + 1.0) * 0.5;               // Use this for OpenGL!
}

float computeLinearDepth(vec3 pos)
{
	vec4 clip_space_pos = Input.v_Proj * Input.v_View * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * v_Near * v_Far) / (v_Far + v_Near - clip_space_depth * (v_Far - v_Near)); // get linear value between 0.01 and 100
    
    //return linearDepth / v_Far; // normalize                        // Use this for Vulkan!!!
    return v_Near * v_Far / (v_Far + linearDepth * (v_Near - v_Far)); // Use this for OpenGL!
}

void main()
{
	float t = -Input.v_NearPoint.y / (Input.v_FarPoint.y - Input.v_NearPoint.y);
	vec3 pos = Input.v_NearPoint + t * (Input.v_FarPoint - Input.v_NearPoint);

    float gradient = 35.0f;
	float scale = 1.0f;

    vec3 fragPos3D = Input.v_NearPoint + t * (Input.v_FarPoint - Input.v_NearPoint);
    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(pos);
	float fading = max(0, (1.0f - gradient * linearDepth));

	o_Color = (grid(pos, scale) + grid(pos, scale / 10.0)) * float(t > 0.0f);
    
    o_Color.a *= fading;
    
    if (o_Color.a <= 0.001)
		discard;

	o_EntityID = v_EntityID;
}
