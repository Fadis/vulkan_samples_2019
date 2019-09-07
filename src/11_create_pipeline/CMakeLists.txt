add_executable( create_pipeline create_pipeline.cpp )
target_link_libraries( create_pipeline vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_pipeline_capi create_pipeline_capi.cpp )
target_link_libraries( create_pipeline_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${BOOST_FILESYSTEM_LIBRARIES} )
