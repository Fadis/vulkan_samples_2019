add_executable( create_sampler create_sampler.cpp )
target_link_libraries( create_sampler vulkan_sample_common ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_sampler_capi create_sampler_capi.cpp )
target_link_libraries( create_sampler_capi vulkan_sample_common
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
