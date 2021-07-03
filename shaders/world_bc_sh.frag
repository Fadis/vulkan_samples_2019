#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "io.h"
#include "constants.h"
#include "push_constants.h"
#include "lighting.h"
#include "shadow.h"

layout(binding = 1) uniform sampler2D base_color;

void main()  {
  vec3 normal = normalize( input_normal.xyz );
  vec3 pos = input_position.xyz;
  vec3 N = normal;
  vec3 V = normalize(dynamic_uniforms.eye_pos.xyz-pos);
  vec3 L = normalize(dynamic_uniforms.light_pos.xyz-pos);
  float roughness = uniforms.roughness;
  float metallicness = uniforms.metalness;
  vec4 diffuse_color = texture( base_color, input_texcoord ) * uniforms.base_color;
  float ambient = 0.05;
  vec3 emissive = uniforms.emissive.rgb;
  float sh = shadow( input_shadow0 );
  vec3 linear = light_with_mask( L, V, N, diffuse_color.rgb, roughness, metallicness, ambient, emissive, dynamic_uniforms.light_energy, sh );
  output_color = vec4( gamma(linear), diffuse_color.a );
}

