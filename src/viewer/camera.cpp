#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <viewer/camera.h>
namespace viewer {
  cameras_t create_camera(
    const fx::gltf::Document &doc,
    float aspect_ratio
  ) {
    cameras_t cameras;
    static const auto lhrh = glm::mat4(-1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1);
    for( const auto &c : doc.cameras ) {
      if( c.type == fx::gltf::Camera::Type::Perspective ) {
        cameras.emplace_back(
          camera_t()
            .set_projection_matrix(
              lhrh *
              glm::perspective(
                c.perspective.yfov,
                ( aspect_ratio == 0.f ) ? c.perspective.aspectRatio : aspect_ratio,
                c.perspective.znear,
                c.perspective.zfar
              )
            )
        );
      }
      else if( c.type == fx::gltf::Camera::Type::Orthographic ) {
        cameras.emplace_back(
          camera_t()
            .set_projection_matrix(
              lhrh *
              glm::ortho(
                -c.orthographic.xmag,
                c.orthographic.xmag,
                -c.orthographic.ymag,
                c.orthographic.ymag,
                -c.orthographic.znear,
                c.orthographic.zfar
              )
            )
        );
      }
    }
    return cameras;
  }
}

