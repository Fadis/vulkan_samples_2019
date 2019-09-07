#ifndef VULKAN_SAMPLE_INCLUDE_COMMON_SETTER_H
#define VULKAN_SAMPLE_INCLUDE_COMMON_SETTER_H

#define VULKAN_SAMPLE_SET_SMALL_VALUE( name ) \
  template< typename T > \
  auto set_ ## name ( T v ) { \
    name = v; \
    return *this; \
  }
#define VULKAN_SAMPLE_SET_LARGE_VALUE( name ) \
  template< typename T > \
  auto set_ ## name ( const T &v ) { \
    name = v; \
    return *this; \
  } \
  template< typename T > \
  auto set_ ## name ( T &&v ) { \
    name = std::move( v ); \
    return *this; \
  }

#endif
