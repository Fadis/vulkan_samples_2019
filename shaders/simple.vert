#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 input_position;
layout (location = 1) in vec3 input_normal;
layout (location = 2) in vec3 input_tangent;
layout (location = 3) in vec2 input_texcoord0;

layout(push_constant) uniform PushConstants {
  mat4 world_matrix;
  mat4 projection_matrix;
} push_constants;

layout (location = 0) out vec4 output_position;
layout (location = 1) out vec4 output_normal;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() {
  vec4 pos = push_constants.world_matrix * vec4( input_position.xyz, 1.0 );
  output_position = pos;
  output_normal = vec4( normalize( ( push_constants.world_matrix * vec4( input_normal.xyz, 1.0 ) ).xyz ), 1.0 );
  gl_Position = push_constants.projection_matrix * pos;
}

