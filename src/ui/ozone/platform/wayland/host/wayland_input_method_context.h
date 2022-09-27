// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_INPUT_METHOD_CONTEXT_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_INPUT_METHOD_CONTEXT_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/strings/string_piece.h"
#include "ui/base/ime/character_composer.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/base/ime/virtual_keyboard_controller.h"
#include "ui/gfx/range/range.h"
#include "ui/ozone/platform/wayland/host/wayland_keyboard.h"
#include "ui/ozone/platform/wayland/host/wayland_window_observer.h"
#include "ui/ozone/platform/wayland/host/zwp_text_input_wrapper.h"

namespace ui {

class WaylandConnection;
class ZWPTextInputWrapper;

class WaylandInputMethodContext : public LinuxInputMethodContext,
                                  public VirtualKeyboardController,
                                  public WaylandWindowObserver,
                                  public ZWPTextInputWrapperClient {
 public:
  class Delegate;

  WaylandInputMethodContext(WaylandConnection* connection,
                            WaylandKeyboard::Delegate* key_delegate,
                            LinuxInputMethodContextDelegate* ime_delegate);
  WaylandInputMethodContext(const WaylandInputMethodContext&) = delete;
  WaylandInputMethodContext& operator=(const WaylandInputMethodContext&) =
      delete;
  ~WaylandInputMethodContext() override;

  void Init(bool initialize_for_testing = false);

  // LinuxInputMethodContext overrides:
  bool DispatchKeyEvent(const ui::KeyEvent& key_event) override;
  // Returns true if this event comes from extended_keyboard::peek_key.
  // See also WaylandEventSource::OnKeyboardKeyEvent about how the flag is set.
  bool IsPeekKeyEvent(const ui::KeyEvent& key_event) override;
  void SetCursorLocation(const gfx::Rect& rect) override;
  void SetSurroundingText(const std::u16string& text,
                          const gfx::Range& selection_range) override;
  void SetContentType(TextInputType type,
                      TextInputMode mode,
                      uint32_t flags,
                      bool should_do_learning) override;
  void UpdateFocus(bool has_client,
                   TextInputType old_type,
                   TextInputType new_type) override;
  void SetGrammarFragmentAtCursor(const GrammarFragment& fragment) override;
  void SetAutocorrectInfo(const gfx::Range& autocorrect_range,
                          const gfx::Rect& autocorrect_bounds) override;
  void Reset() override;
  VirtualKeyboardController* GetVirtualKeyboardController() override;

  // VirtualKeyboardController overrides:
  bool DisplayVirtualKeyboard() override;
  void DismissVirtualKeyboard() override;
  void AddObserver(VirtualKeyboardControllerObserver* observer) override;
  void RemoveObserver(VirtualKeyboardControllerObserver* observer) override;
  bool IsKeyboardVisible() override;

  // WaylandWindowObserver overrides:
  void OnKeyboardFocusedWindowChanged() override;

  // ZWPTextInputWrapperClient overrides:
  void OnPreeditString(base::StringPiece text,
                       const std::vector<SpanStyle>& spans,
                       int32_t preedit_cursor) override;
  void OnCommitString(base::StringPiece text) override;
  void OnDeleteSurroundingText(int32_t index, uint32_t length) override;
  void OnKeysym(uint32_t keysym, uint32_t state, uint32_t modifiers) override;
  void OnSetPreeditRegion(int32_t index,
                          uint32_t length,
                          const std::vector<SpanStyle>& spans) override;

  void OnClearGrammarFragments(const gfx::Range& range) override;
  void OnAddGrammarFragment(const GrammarFragment& fragment) override;
  void OnSetAutocorrectRange(const gfx::Range& range) override;

  void OnInputPanelState(uint32_t state) override;
  void OnModifiersMap(std::vector<std::string> modifiers_map) override;

 private:
  void Focus();
  void Blur();
  void UpdatePreeditText(const std::u16string& preedit_text);
  void MaybeUpdateActivated();

  const raw_ptr<WaylandConnection>
      connection_;  // TODO(jani) Handle this better

  // Delegate key events to be injected into PlatformEvent system.
  const raw_ptr<WaylandKeyboard::Delegate> key_delegate_;

  // Delegate IME-specific events to be handled by //ui code.
  const raw_ptr<LinuxInputMethodContextDelegate> ime_delegate_;

  std::unique_ptr<ZWPTextInputWrapper> text_input_;

  // Tracks whether InputMethod in Chrome has some focus.
  bool focused_ = false;

  // Tracks whether a request to activate InputMethod is sent to wayland
  // compositor.
  bool activated_ = false;

  // An object to compose a character from a sequence of key presses
  // including dead key etc.
  CharacterComposer character_composer_;

  // Stores the parameters required for OnDeleteSurroundingText.
  // The index moved by SetSurroundingText. This is byte-offset in UTF8 form.
  size_t surrounding_text_offset_ = 0;
  // The string in SetSurroundingText.
  std::string surrounding_text_;
  // The selection range in UTF-8 offsets in the |surrounding_text_|.
  gfx::Range selection_range_utf8_ = gfx::Range::InvalidRange();

  // Caches VirtualKeyboard visibility.
  bool virtual_keyboard_visible_ = false;

  // Keeps modifiers_map sent from the wayland compositor.
  std::vector<std::string> modifiers_map_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_INPUT_METHOD_CONTEXT_H_
