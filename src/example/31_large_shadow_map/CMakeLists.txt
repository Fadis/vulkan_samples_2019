add_executable( large_shadow_map large_shadow_map.cpp )
target_link_libraries(
  large_shadow_map
  vw
  viewer
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  ${OIIO_LIBRARIES}
)

