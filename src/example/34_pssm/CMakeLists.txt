add_executable( pssm pssm.cpp )
target_link_libraries(
  pssm
  vw
  viewer
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  ${OIIO_LIBRARIES}
)

