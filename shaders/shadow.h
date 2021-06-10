layout(binding = 6) uniform sampler2D shadow;

float simple_shadow( vec4 pos ) {
  vec4 proj_pos = pos;
  proj_pos /= proj_pos.w;
  float shadow_distance = max( ( texture( shadow, proj_pos.xy * 0.5 + 0.5 ).r ), 0.0 );
  if( shadow_distance == 0.f ) return 0.0;
  float distance = proj_pos.z - 0.001;
  if( shadow_distance < distance ) return 1.0;
  else return 0.0;
}

