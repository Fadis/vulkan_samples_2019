add_executable( psm psm.cpp )
target_link_libraries(
  psm
  vw
  viewer
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  ${OIIO_LIBRARIES}
)

