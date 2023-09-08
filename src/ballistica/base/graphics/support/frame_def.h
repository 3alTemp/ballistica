// Released under the MIT License. See LICENSE for details.

#ifndef BALLISTICA_BASE_GRAPHICS_SUPPORT_FRAME_DEF_H_
#define BALLISTICA_BASE_GRAPHICS_SUPPORT_FRAME_DEF_H_

#include <memory>
#include <vector>

#include "ballistica/base/assets/asset.h"
#include "ballistica/shared/math/matrix44f.h"
#include "ballistica/shared/math/vector2f.h"

namespace ballistica::base {

/// A flattened representation of a frame; generated by the logic thread and
/// sent to the graphics thread to render.
class FrameDef {
 public:
  auto light_pass() -> RenderPass* { return light_pass_.get(); }
  auto light_shadow_pass() -> RenderPass* { return light_shadow_pass_.get(); }
  auto beauty_pass() -> RenderPass* { return beauty_pass_.get(); }
  auto beauty_pass_bg() -> RenderPass* { return beauty_pass_bg_.get(); }
  auto overlay_pass() -> RenderPass* { return overlay_pass_.get(); }
  auto overlay_front_pass() -> RenderPass* { return overlay_front_pass_.get(); }
  auto vr_near_clip() const -> float { return vr_near_clip_; }
  void set_vr_near_clip(float val) { vr_near_clip_ = val; }
  auto benchmark_type() const -> BenchmarkType { return benchmark_type_; }
  void set_benchmark_type(BenchmarkType val) { benchmark_type_ = val; }

  // Returns the fixed overlay pass if there is one; otherwise the regular.
  auto GetOverlayFixedPass() -> RenderPass*;

  // Return either the overlay-flat pass (in vr) or regular overlay pass (for
  // non-vr).
  auto GetOverlayFlatPass() -> RenderPass*;
  auto overlay_3d_pass() -> RenderPass* { return overlay_3d_pass_.get(); }
  auto blit_pass() -> RenderPass* { return blit_pass_.get(); }
  auto vr_cover_pass() -> RenderPass* { return vr_cover_pass_.get(); }

  // Returns the real-time this frame_def originated at.
  // For a more smoothly-incrementing value,
  // use getbasetime()
  auto real_time() const -> millisecs_t { return app_time_millisecs_; }
  auto frame_number() const -> int64_t { return frame_number_; }

  // Returns the bsGame master-net-time when this was made
  // (tries to match real time but is incremented more smoothly
  // so is better for drawing purposes)
  auto display_time_millisecs() const -> millisecs_t {
    return display_time_millisecs_;
  }

  // How much base time does this frame-def represent.
  auto display_time_elapsed_millisecs() const -> millisecs_t {
    return display_time_elapsed_millisecs_;
  }

  auto quality() const -> GraphicsQuality { return quality_; }
  auto orbiting() const -> bool { return orbiting_; }
  auto shadow_offset() const -> const Vector3f& { return shadow_offset_; }
  auto shadow_scale() const -> const Vector2f& { return shadow_scale_; }
  auto shadow_ortho() const -> bool { return shadow_ortho_; }
  auto tint() const -> const Vector3f& { return tint_; }
  auto ambient_color() const -> const Vector3f& { return ambient_color_; }
  auto vignette_outer() const -> const Vector3f& { return vignette_outer_; }
  auto vignette_inner() const -> const Vector3f& { return vignette_inner_; }

  // FIXME: what was this for?..(I think some vr thing?)
  auto cam_original() const -> const Vector3f& { return cam_original_; }
  auto cam_target_original() const -> const Vector3f& {
    return cam_target_original_;
  }
  void set_cam_original(const Vector3f& val) { cam_original_ = val; }
  void set_cam_target_original(const Vector3f& val) {
    cam_target_original_ = val;
  }
  auto camera_mode() const -> CameraMode { return camera_mode_; }
  auto vr_overlay_screen_matrix() const -> const Matrix44f& {
    return vr_overlay_screen_matrix_;
  }
  void set_vr_overlay_screen_matrix(const Matrix44f& mat) {
    vr_overlay_screen_matrix_ = mat;
  }
  auto vr_overlay_screen_matrix_fixed() const -> const Matrix44f& {
    return vr_overlay_screen_matrix_fixed_;
  }
  void set_vr_overlay_screen_matrix_fixed(const Matrix44f& mat) {
    vr_overlay_screen_matrix_fixed_ = mat;
  }

  // Effects requiring availability of a depth texture should
  // check this to determine whether they should draw.
  auto has_depth_texture() const -> bool {
    return (quality_ >= GraphicsQuality::kHigh);
  }
  void AddComponent(const Object::Ref<Asset>& component) {
    // Add a reference to this component only if we havn't yet.
    if (component->last_frame_def_num() != frame_number_) {
      component->set_last_frame_def_num(frame_number_);
      media_components_.push_back(component);
    }
  }
  void AddMesh(Mesh* mesh);
  void set_needs_clear(bool val) { needs_clear_ = val; }
  auto needs_clear() const -> bool { return needs_clear_; }

  FrameDef();
  ~FrameDef();
  void Reset();
  void Finalize();

  void set_display_time_elapsed_millisecs(millisecs_t val) {
    display_time_elapsed_millisecs_ = val;
  }
  void set_app_time_millisecs(millisecs_t val) { app_time_millisecs_ = val; }
  void set_display_time_millisecs(millisecs_t val) {
    display_time_millisecs_ = val;
  }
  void set_frame_number(int64_t val) { frame_number_ = val; }

  auto overlay_flat_pass() const -> RenderPass* {
    return overlay_flat_pass_.get();
  }
  auto overlay_fixed_pass() const -> RenderPass* {
    return overlay_fixed_pass_.get();
  }
  auto overlay_front_pass() const -> RenderPass* {
    return overlay_front_pass_.get();
  }
  auto overlay_pass() const -> RenderPass* { return overlay_pass_.get(); }
  auto vr_cover_pass() const -> RenderPass* { return vr_cover_pass_.get(); }

  void set_mesh_data_creates(const std::vector<MeshData*>& creates) {
    mesh_data_creates_ = creates;
  }
  void set_mesh_data_destroys(const std::vector<MeshData*>& destroys) {
    mesh_data_destroys_ = destroys;
  }
  auto mesh_data_creates() const -> const std::vector<MeshData*>& {
    return mesh_data_creates_;
  }
  auto mesh_data_destroys() const -> const std::vector<MeshData*>& {
    return mesh_data_destroys_;
  }
  auto meshes() const -> const std::vector<Object::Ref<MeshDataClientHandle>>& {
    return meshes_;
  }
  auto mesh_buffers() const -> const std::vector<Object::Ref<MeshBufferBase>>& {
    return mesh_buffers_;
  }
  auto mesh_index_sizes() const -> const std::vector<int8_t>& {
    return mesh_index_sizes_;
  }
  auto media_components() const -> const std::vector<Object::Ref<Asset>>& {
    return media_components_;
  }
  auto tv_border() const { return tv_border_; }

  void set_camera_mode(CameraMode val) { camera_mode_ = val; }
  void set_rendering(bool val) { rendering_ = val; }
  auto rendering() const -> bool { return rendering_; }
  void set_shake_original(const Vector3f& val) { shake_original_ = val; }
  auto shake_original() const -> const Vector3f& { return shake_original_; }

#if BA_DEBUG_BUILD
  auto defining_component() const -> bool { return defining_component_; }
  void set_defining_component(bool val) { defining_component_ = val; }
#endif

 private:
  bool needs_clear_{};
  BenchmarkType benchmark_type_{BenchmarkType::kNone};
  bool rendering_{};
  CameraMode camera_mode_{CameraMode::kFollow};
  Vector3f cam_original_{0.0f, 0.0f, 0.0f};
  Vector3f cam_target_original_{0.0f, 0.0f, 0.0f};
  Vector3f shake_original_{0.0f, 0.0f, 0.0f};
  float vr_near_clip_{};
  Matrix44f vr_overlay_screen_matrix_ = kMatrix44fIdentity;
  Matrix44f vr_overlay_screen_matrix_fixed_ = kMatrix44fIdentity;
  std::vector<MeshData*> mesh_data_creates_;
  std::vector<MeshData*> mesh_data_destroys_;

  // Meshes/Buffers:
  std::vector<Object::Ref<MeshDataClientHandle>> meshes_;
  std::vector<Object::Ref<MeshBufferBase>> mesh_buffers_;
  std::vector<int8_t> mesh_index_sizes_;
  std::vector<Object::Ref<Asset>> media_components_;

#if BA_DEBUG_BUILD
  // Sanity checking: make sure components are completely submitted
  // before new ones are started (so we dont get scrambled command buffers).
  bool defining_component_{};
#endif

  std::unique_ptr<RenderPass> light_pass_;
  std::unique_ptr<RenderPass> light_shadow_pass_;
  std::unique_ptr<RenderPass> beauty_pass_;
  std::unique_ptr<RenderPass> beauty_pass_bg_;
  std::unique_ptr<RenderPass> overlay_pass_;
  std::unique_ptr<RenderPass> overlay_front_pass_;
  std::unique_ptr<RenderPass> overlay_fixed_pass_;
  std::unique_ptr<RenderPass> overlay_flat_pass_;
  std::unique_ptr<RenderPass> vr_cover_pass_;
  std::unique_ptr<RenderPass> overlay_3d_pass_;
  std::unique_ptr<RenderPass> blit_pass_;
  GraphicsQuality quality_{GraphicsQuality::kLow};
  bool orbiting_{};
  bool tv_border_{};
  millisecs_t app_time_millisecs_{};
  millisecs_t display_time_millisecs_{};
  millisecs_t display_time_elapsed_millisecs_{};
  int64_t frame_number_{};
  Vector3f shadow_offset_{0.0f, 0.0f, 0.0f};
  Vector2f shadow_scale_{1.0f, 1.0f};
  bool shadow_ortho_{};
  Vector3f tint_{1.0f, 1.0f, 1.0f};
  Vector3f ambient_color_{1.0f, 1.0f, 1.0f};
  Vector3f vignette_outer_{1.0f, 1.0f, 1.0f};
  Vector3f vignette_inner_{1.0f, 1.0f, 1.0f};
};

}  // namespace ballistica::base

#endif  // BALLISTICA_BASE_GRAPHICS_SUPPORT_FRAME_DEF_H_
