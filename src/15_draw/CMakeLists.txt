add_executable( draw draw.cpp )
target_link_libraries( draw vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( draw_capi draw_capi.cpp )
target_link_libraries( draw_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
