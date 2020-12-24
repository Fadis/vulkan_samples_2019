add_executable( create_render_pass create_render_pass.cpp )
target_link_libraries( create_render_pass vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_render_pass_capi create_render_pass_capi.cpp )
target_link_libraries( create_render_pass_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
