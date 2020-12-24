add_executable( create_swapchain create_swapchain.cpp )
target_link_libraries( create_swapchain vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_swapchain_capi create_swapchain_capi.cpp )
target_link_libraries( create_swapchain_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
