add_executable( device_group device_group.cpp )
target_link_libraries( device_group vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( device_group_capi device_group_capi.cpp )
target_link_libraries( device_group_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
