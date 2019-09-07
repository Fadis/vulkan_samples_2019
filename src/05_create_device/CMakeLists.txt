add_executable( create_device create_device.cpp )
target_link_libraries( create_device vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_device_capi create_device_capi.cpp )
target_link_libraries( create_device_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
