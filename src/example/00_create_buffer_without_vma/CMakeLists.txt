add_executable( create_buffer_without_vma create_buffer_without_vma.cpp )
target_link_libraries( create_buffer_without_vma vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_buffer_without_vma_capi create_buffer_without_vma_capi.cpp )
target_link_libraries( create_buffer_without_vma_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
