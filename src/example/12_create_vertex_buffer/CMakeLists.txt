add_executable( create_vertex_buffer create_vertex_buffer.cpp )
target_link_libraries( create_vertex_buffer vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_vertex_buffer_capi create_vertex_buffer_capi.cpp )
target_link_libraries( create_vertex_buffer_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
