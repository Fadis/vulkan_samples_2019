add_executable( create_window create_window.cpp )
target_link_libraries( create_window vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_window_capi create_window_capi.cpp )
target_link_libraries( create_window_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
