// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_X11_X11_OZONE_UI_CONTROLS_TEST_HELPER_H_
#define UI_OZONE_PLATFORM_X11_X11_OZONE_UI_CONTROLS_TEST_HELPER_H_

#include "ui/ozone/public/ozone_ui_controls_test_helper.h"

#include "ui/base/x/test/x11_ui_controls_test_helper.h"

namespace ui {

class X11OzoneUIControlsTestHelper : public OzoneUIControlsTestHelper {
 public:
  X11OzoneUIControlsTestHelper();
  X11OzoneUIControlsTestHelper(const X11OzoneUIControlsTestHelper&) = delete;
  X11OzoneUIControlsTestHelper& operator=(const X11OzoneUIControlsTestHelper&) =
      delete;
  ~X11OzoneUIControlsTestHelper() override;

  unsigned ButtonDownMask() const override;
  void SendKeyPressEvent(gfx::AcceleratedWidget widget,
                         ui::KeyboardCode key,
                         bool control,
                         bool shift,
                         bool alt,
                         bool command,
                         base::OnceClosure closure) override;
  void SendMouseMotionNotifyEvent(gfx::AcceleratedWidget widget,
                                  const gfx::Point& mouse_loc,
                                  const gfx::Point& mouse_root_loc,
                                  base::OnceClosure closure) override;
  void SendMouseEvent(gfx::AcceleratedWidget widget,
                      ui_controls::MouseButton type,
                      int button_state,
                      int accelerator_state,
                      const gfx::Point& mouse_loc,
                      const gfx::Point& mouse_root_loc,
                      base::OnceClosure closure) override;
  void RunClosureAfterAllPendingUIEvents(base::OnceClosure closure) override;
  bool MustUseUiControlsForMoveCursorTo() override;

 private:
  X11UIControlsTestHelper x11_ui_controls_test_helper_;
};

OzoneUIControlsTestHelper* CreateOzoneUIControlsTestHelperX11();

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_X11_X11_OZONE_UI_CONTROLS_TEST_HELPER_H_
