layout(push_constant) uniform PushConstants {
  mat4 world_matrix;
} push_constants;

layout(binding = 0) uniform Uniforms {
  vec4 base_color;
  vec4 emissive;
  float roughness;
  float metalness;
  float normal_scale;
  float occlusion_strength;
} uniforms;

layout(binding = 7) uniform DynamicUniforms {
  mat4 projection_matrix;
  mat4 camera_matrix;
  mat4 light_matrix;
  vec4 eye_pos;
  vec4 light_pos;
  float light_energy;
} dynamic_uniforms;

