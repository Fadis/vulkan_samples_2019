/*
 * Copyright (C) 2020 Naomasa Matsubayashi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <fx/gltf.h>
#include <vw/image.h>
#include <vw/command_buffer.h>
#include <viewer/image.h>
namespace viewer {
  images_t create_image(
    const fx::gltf::Document &doc,
    const vw::context_t &context,
    const std::filesystem::path cd
  ) {
    images_t images;
    unsigned int cur = 1u;
    for( const auto &image: doc.images ) {
      auto image_path = std::filesystem::path( image.uri );
      if( image_path.is_relative() ) image_path = cd / image_path;
      images.push_back( image_t() );
      std::cout << "[" << cur << "/" << doc.images.size() <<  "] " << image_path.string() << " をロード中..." << std::flush;
      const vk::UniqueHandle< vk::Semaphore, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > null_semaphore;
      images.back().set_unorm(
        vw::load_image( context, image_path.string(), vk::ImageUsageFlagBits::eSampled, true, false )
      );
      images.back().set_srgb(
        vw::load_image( context, image_path.string(), vk::ImageUsageFlagBits::eSampled, true, true )
      );
      std::cout << " OK" << std::endl;
      ++cur;
    }
    return images;
  }
}

