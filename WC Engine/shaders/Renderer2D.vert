#pragma shader_stage(vertex)

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

struct Vertex
{
	vec3 Position;
	uint TextureID;
	vec2 TexCoords;
	float Fade;
	float Thickness;

	vec4 Color;
};

layout(buffer_reference, scalar) buffer VertexBufferPointer { Vertex vertices[]; };

layout (push_constant) uniform Data
{
	mat4 ViewProj;
    VertexBufferPointer vbp;
};

layout(location = 0) out vec2 v_TexCoords;
layout(location = 1) out flat uint v_TexID;
layout(location = 2) out vec4 v_Color;
layout(location = 3) out float v_Fade;
layout(location = 4) out float v_Thickness;

void main()
{
    Vertex vertex = vbp.vertices[gl_VertexIndex];

    v_TexCoords = vertex.TexCoords;
	v_TexID = vertex.TextureID;
	v_Color = vertex.Color;
	v_Fade = vertex.Fade;
	v_Thickness = vertex.Thickness;

    gl_Position = ViewProj * vec4(vertex.Position, 1.f);
}
