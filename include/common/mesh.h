#ifndef VULKAN_SAMPLES_INCLUDE_MESH_H
#define VULKAN_SAMPLES_INCLUDE_MESH_H
#include <cstddef>
#include <vector>
#include <string>
#include <boost/container/flat_map.hpp>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <common/vertex.h>
#include <common/mesh.h>
namespace common {
  struct unable_to_load_mesh {};
  struct mesh_offset {
    mesh_offset( size_t b, size_t c, size_t m ) : vertex_begin( b ), vertex_count( c ), matrix( m ) {}
    size_t vertex_begin;
    size_t vertex_count;
    size_t matrix;
  };
  class scene {
  public:
    scene( const std::string &filename );
    const std::vector< vertex_t > &get_vertices() const { return vertices; }
    const std::vector< glm::mat4 > &get_matrices() const { return matrices; }
    const std::vector< mesh_offset > &get_offsets() const { return offsets; }
    const std::vector< glm::mat4 > &get_cameras() const { return cameras; }
    const std::vector< glm::vec3 > &get_camera_pos() const { return camera_pos; }
  private:
    boost::container::flat_map< std::string, aiCamera* > load_cameras( const aiScene &scene );
    void load_camera( glm::mat4 world, const aiCamera &camera );
    void generate_default_camera();
    void load_node( const aiNode &node, const glm::mat4 &coord, const std::vector< std::pair< size_t, size_t > > &vertex_offsets, const boost::container::flat_map< std::string, aiCamera* > &cameras_ );
    std::vector< std::pair< size_t, size_t > > load_meshes( const aiScene &scene );
    std::pair< size_t, size_t > load_mesh( const aiMesh &mesh );
    void load_face( const aiMesh &mesh, uint32_t index );
    void load_vertex( const aiMesh &mesh, uint32_t index );
    std::vector< vertex_t > vertices;
    std::vector< glm::mat4 > matrices;
    std::vector< mesh_offset > offsets;
    std::vector< glm::mat4 > cameras;
    std::vector< glm::vec3 > camera_pos;
  };
}
#endif

