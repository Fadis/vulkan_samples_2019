add_executable( select_device select_device.cpp )
target_link_libraries( select_device vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( select_device_capi select_device_capi.cpp )
target_link_libraries( select_device_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
