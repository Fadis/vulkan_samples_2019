#ifndef VIEWER_TEXTURE_H
#define VIEWER_TEXTURE_H
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
#include <vector>
#include <vulkan/vulkan.hpp>
#include <fx/gltf.h>
#include <stamp/setter.h>
#include <vw/context.h>
#include <viewer/image.h>
#include <viewer/sampler.h>
namespace viewer {
  struct texture_t {
    LIBSTAMP_SETTER( unorm )
    LIBSTAMP_SETTER( srgb )
    vk::DescriptorImageInfo unorm;
    vk::DescriptorImageInfo srgb;
  };
  using textures_t = std::vector< texture_t >;
  texture_t create_texture(
    const fx::gltf::Document &doc,
    int32_t index,
    const vw::context_t &context,
    const images_t &images,
    const samplers_t &samplers,
    const sampler_t &default_samplers
  );
  texture_t create_texture(
    int32_t index,
    const vw::context_t &context,
    const image_t &image,
    const samplers_t &samplers,
    const sampler_t &default_samplers
  );
  texture_t create_texture(
    const vw::image_t &image,
    const sampler_t &sampler
  );
  textures_t create_texture(
    const fx::gltf::Document &doc,
    const vw::context_t &context,
    const images_t &images,
    const samplers_t &samplers,
    const sampler_t &default_samplers
  );
}
#endif

