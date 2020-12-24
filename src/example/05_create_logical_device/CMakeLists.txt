add_executable( create_logical_device create_logical_device.cpp )
target_link_libraries( create_logical_device vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_logical_device_capi create_logical_device_capi.cpp )
target_link_libraries( create_logical_device_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
