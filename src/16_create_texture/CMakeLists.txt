add_executable( create_texture create_texture.cpp )
target_link_libraries( create_texture vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_texture_capi create_texture_capi.cpp )
target_link_libraries( create_texture_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
