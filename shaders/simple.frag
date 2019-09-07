#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (location = 0) in vec4 input_position;
layout (location = 1) in vec4 input_normal;
layout (location = 0) out vec4 output_color;



layout(push_constant) uniform PushConstants {
  mat4 world_matrix;
  mat4 projection_matrix;
  vec3 eye;
  vec3 lightpos;
} push_constants;


float ggx (vec3 N, vec3 V, vec3 L, float roughness, float F0) {
  float alpha = roughness*roughness;
  vec3 H = normalize(L - V);
  float dotLH = max( 0.0, float( dot(L,H) ) );
  float dotNH = max( 0.0, float( dot(N,H) ) );
  float dotNL = max( 0.0, float( dot(N,L) ) );
  float alphaSqr = alpha * alpha;
  float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
  float D = alphaSqr / (3.141592653589793 * denom * denom);
  float F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 80.0);
  float k = 0.5 * alpha;
  float k2 = k * k;
  return dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);
}

void main()  {
  vec3 pos = input_position.xyz;
  vec3 N = input_normal.xyz;
  vec3 V = normalize(push_constants.eye-pos);
  vec3 L = normalize(push_constants.lightpos-pos);
  float spec = max(0.0,ggx(N,V,L,0.8,0.5));
  vec3 c = vec3( 1, 1, 1 );
  output_color = vec4(pow(c*(spec),vec3(2.2)),1);
}

