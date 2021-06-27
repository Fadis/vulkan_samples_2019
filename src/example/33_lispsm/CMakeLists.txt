add_executable( lispsm lispsm.cpp )
target_link_libraries(
  lispsm
  vw
  viewer
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${Vulkan_LIBRARIES}
  ${OIIO_LIBRARIES}
)

