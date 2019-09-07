GLSLC=glslc #~/vulkansdk_old/1.1.92.1/x86_64/bin/glslc
${GLSLC} simple.vert -o simple.vert.spv --target-env=vulkan1.1
${GLSLC} simple.frag -o simple.frag.spv --target-env=vulkan1.1
