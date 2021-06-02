add_executable( shadow_map shadow_map.cpp )
target_link_libraries(
  shadow_map
  vw
  viewer
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  ${OIIO_LIBRARIES}
)

