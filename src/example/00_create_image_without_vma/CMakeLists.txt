add_executable( create_image_without_vma create_image_without_vma.cpp )
target_link_libraries( create_image_without_vma vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_image_without_vma_capi create_image_without_vma_capi.cpp )
target_link_libraries( create_image_without_vma_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
