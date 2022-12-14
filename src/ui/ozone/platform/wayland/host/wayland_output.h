// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_OUTPUT_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_OUTPUT_H_

#include <stdint.h>

#include "base/memory/raw_ptr.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class XDGOutput;
class WaylandZcrColorManager;
class WaylandZcrColorManagementOutput;
class WaylandConnection;
class WaylandZAuraOutput;

// WaylandOutput objects keep track of the current output of display
// that are available to the application.
class WaylandOutput : public wl::GlobalObjectRegistrar<WaylandOutput> {
 public:
  static constexpr char kInterfaceName[] = "wl_output";

  static void Instantiate(WaylandConnection* connection,
                          wl_registry* registry,
                          uint32_t name,
                          const std::string& interface,
                          uint32_t version);

  class Delegate {
   public:
    virtual void OnOutputHandleMetrics(uint32_t output_id,
                                       const gfx::Point& origin,
                                       const gfx::Size& logical_size,
                                       const gfx::Size& physical_size,
                                       const gfx::Insets& insets,
                                       float scale_factor,
                                       int32_t panel_transform,
                                       int32_t logical_transform,
                                       const std::string& label) = 0;

   protected:
    virtual ~Delegate() = default;
  };

  WaylandOutput(uint32_t output_id,
                wl_output* output,
                WaylandConnection* connection);

  WaylandOutput(const WaylandOutput&) = delete;
  WaylandOutput& operator=(const WaylandOutput&) = delete;

  ~WaylandOutput();

  void Initialize(Delegate* delegate);
  void InitializeXdgOutput(struct zxdg_output_manager_v1* manager);
  void InitializeZAuraOutput(zaura_shell* aura_shell);
  void InitializeColorManagementOutput(WaylandZcrColorManager* manager);
  float GetUIScaleFactor() const;

  uint32_t output_id() const { return output_id_; }
  bool has_output(wl_output* output) const { return output_.get() == output; }
  float scale_factor() const { return scale_factor_; }
  int32_t panel_transform() const { return panel_transform_; }
  int32_t logical_transform() const;
  gfx::Point origin() const;
  gfx::Size logical_size() const;
  gfx::Size physical_size() const { return physical_size_; }
  gfx::Insets insets() const;
  const std::string& label() const;
  WaylandZcrColorManagementOutput* color_management_output() const {
    return color_management_output_.get();
  }

  // Tells if the output has already received physical screen dimensions in the
  // global compositor space.
  bool is_ready() const { return !physical_size_.IsEmpty(); }

  wl_output* get_output() { return output_.get(); }
  zaura_output* get_zaura_output();

 private:
  static constexpr int32_t kDefaultScaleFactor = 1;

  void TriggerDelegateNotifications();

  // Callback functions used for setting geometric properties of the output
  // and available modes.
  static void OutputHandleGeometry(void* data,
                                   wl_output* output,
                                   int32_t x,
                                   int32_t y,
                                   int32_t physical_width,
                                   int32_t physical_height,
                                   int32_t subpixel,
                                   const char* make,
                                   const char* model,
                                   int32_t output_transform);

  static void OutputHandleMode(void* data,
                               wl_output* wl_output,
                               uint32_t flags,
                               int32_t width,
                               int32_t height,
                               int32_t refresh);
  static void OutputHandleDone(void* data, struct wl_output* wl_output);
  static void OutputHandleScale(void* data,
                                struct wl_output* wl_output,
                                int32_t factor);

  const uint32_t output_id_ = 0;
  wl::Object<wl_output> output_;
  std::unique_ptr<XDGOutput> xdg_output_;
  std::unique_ptr<WaylandZAuraOutput> aura_output_;
  std::unique_ptr<WaylandZcrColorManagementOutput> color_management_output_;
  float scale_factor_ = kDefaultScaleFactor;
  int32_t panel_transform_ = WL_OUTPUT_TRANSFORM_NORMAL;
  // Origin of the output in DIP screen coordinate.
  gfx::Point origin_;
  // Size of the output in physical pixels.
  gfx::Size physical_size_;

  raw_ptr<Delegate> delegate_ = nullptr;
  raw_ptr<WaylandConnection> connection_ = nullptr;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_OUTPUT_H_
