// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/linux/fake_linux_ui.h"

#include "base/time/time.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace ui {

FakeLinuxUi::FakeLinuxUi() = default;

FakeLinuxUi::~FakeLinuxUi() = default;

std::unique_ptr<ui::LinuxInputMethodContext>
FakeLinuxUi::CreateInputMethodContext(
    ui::LinuxInputMethodContextDelegate* delegate) const {
  return nullptr;
}

gfx::FontRenderParams FakeLinuxUi::GetDefaultFontRenderParams() const {
  return gfx::FontRenderParams();
}

void FakeLinuxUi::GetDefaultFontDescription(
    std::string* family_out,
    int* size_pixels_out,
    int* style_out,
    int* weight_out,
    gfx::FontRenderParams* params_out) const {}

ui::SelectFileDialog* FakeLinuxUi::CreateSelectFileDialog(
    void* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy) const {
  return nullptr;
}

bool FakeLinuxUi::Initialize() {
  return false;
}

bool FakeLinuxUi::GetColor(int id,
                           SkColor* color,
                           bool use_custom_frame) const {
  return false;
}

bool FakeLinuxUi::GetDisplayProperty(int id, int* result) const {
  return false;
}

SkColor FakeLinuxUi::GetFocusRingColor() const {
  return gfx::kPlaceholderColor;
}

SkColor FakeLinuxUi::GetActiveSelectionBgColor() const {
  return gfx::kPlaceholderColor;
}

SkColor FakeLinuxUi::GetActiveSelectionFgColor() const {
  return gfx::kPlaceholderColor;
}

SkColor FakeLinuxUi::GetInactiveSelectionBgColor() const {
  return gfx::kPlaceholderColor;
}

SkColor FakeLinuxUi::GetInactiveSelectionFgColor() const {
  return gfx::kPlaceholderColor;
}

base::TimeDelta FakeLinuxUi::GetCursorBlinkInterval() const {
  return base::TimeDelta();
}

gfx::Image FakeLinuxUi::GetIconForContentType(const std::string& content_type,
                                              int size,
                                              float scale) const {
  return gfx::Image();
}

LinuxUi::WindowFrameAction FakeLinuxUi::GetWindowFrameAction(
    WindowFrameActionSource source) {
  return WindowFrameAction::kNone;
}

float FakeLinuxUi::GetDeviceScaleFactor() const {
  return 1.0f;
}

bool FakeLinuxUi::PreferDarkTheme() const {
  return false;
}

bool FakeLinuxUi::AnimationsEnabled() const {
  return true;
}

std::unique_ptr<ui::NavButtonProvider> FakeLinuxUi::CreateNavButtonProvider() {
  return nullptr;
}

ui::WindowFrameProvider* FakeLinuxUi::GetWindowFrameProvider(bool solid_frame) {
  return nullptr;
}

base::flat_map<std::string, std::string> FakeLinuxUi::GetKeyboardLayoutMap() {
  return base::flat_map<std::string, std::string>();
}

std::string FakeLinuxUi::GetCursorThemeName() {
  return std::string();
}

int FakeLinuxUi::GetCursorThemeSize() {
  return 0;
}

ui::NativeTheme* FakeLinuxUi::GetNativeThemeImpl() const {
  return nullptr;
}

bool FakeLinuxUi::GetTextEditCommandsForEvent(
    const ui::Event& event,
    std::vector<ui::TextEditCommandAuraLinux>* commands) {
  return false;
}

#if BUILDFLAG(ENABLE_PRINTING)
printing::PrintDialogLinuxInterface* FakeLinuxUi::CreatePrintDialog(
    printing::PrintingContextLinux* context) {
  return nullptr;
}

gfx::Size FakeLinuxUi::GetPdfPaperSize(
    printing::PrintingContextLinux* context) {
  return gfx::Size();
}
#endif

}  // namespace ui
