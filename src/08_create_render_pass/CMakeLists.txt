add_executable( create_render_pass create_render_pass.cpp )
target_link_libraries( create_render_pass vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_render_pass_capi create_render_pass_capi.cpp )
target_link_libraries( create_render_pass_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
