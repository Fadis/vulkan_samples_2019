add_executable( timeline_semaphore timeline_semaphore.cpp )
target_link_libraries( timeline_semaphore vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( timeline_semaphore_capi timeline_semaphore_capi.cpp )
target_link_libraries( timeline_semaphore_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
