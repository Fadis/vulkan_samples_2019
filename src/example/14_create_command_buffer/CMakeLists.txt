add_executable( create_command_buffer create_command_buffer.cpp )
target_link_libraries( create_command_buffer vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_command_buffer_capi create_command_buffer_capi.cpp )
target_link_libraries( create_command_buffer_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
