add_executable( create_descriptor_set create_descriptor_set.cpp )
target_link_libraries( create_descriptor_set vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_descriptor_set_capi create_descriptor_set_capi.cpp )
target_link_libraries( create_descriptor_set_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
