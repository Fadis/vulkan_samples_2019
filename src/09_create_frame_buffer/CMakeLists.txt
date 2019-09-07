add_executable( create_frame_buffer create_frame_buffer.cpp )
target_link_libraries( create_frame_buffer vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_frame_buffer_capi create_frame_buffer_capi.cpp )
target_link_libraries( create_frame_buffer_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
