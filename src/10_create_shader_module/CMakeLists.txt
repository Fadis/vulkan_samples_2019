add_executable( create_shader_module create_shader_module.cpp )
target_link_libraries( create_shader_module vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_shader_module_capi create_shader_module_capi.cpp )
target_link_libraries( create_shader_module_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
