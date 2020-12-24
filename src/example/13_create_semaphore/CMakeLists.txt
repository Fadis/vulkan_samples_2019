add_executable( create_semaphore create_semaphore.cpp )
target_link_libraries( create_semaphore vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_semaphore_capi create_semaphore_capi.cpp )
target_link_libraries( create_semaphore_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
