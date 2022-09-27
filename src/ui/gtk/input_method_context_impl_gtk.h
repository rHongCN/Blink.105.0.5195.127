// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GTK_INPUT_METHOD_CONTEXT_IMPL_GTK_H_
#define UI_GTK_INPUT_METHOD_CONTEXT_IMPL_GTK_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ui/base/glib/glib_integers.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/gfx/geometry/rect.h"

using GtkIMContext = struct _GtkIMContext;
using GdkWindow = struct _GdkWindow;

namespace gtk {

// An implementation of LinuxInputMethodContext which uses GtkIMContext
// (gtk-immodule) as a bridge from/to underlying IMEs.
class InputMethodContextImplGtk : public ui::LinuxInputMethodContext {
 public:
  explicit InputMethodContextImplGtk(
      ui::LinuxInputMethodContextDelegate* delegate);

  InputMethodContextImplGtk(const InputMethodContextImplGtk&) = delete;
  InputMethodContextImplGtk& operator=(const InputMethodContextImplGtk&) =
      delete;

  ~InputMethodContextImplGtk() override;

  // Overridden from ui::LinuxInputMethodContext
  bool DispatchKeyEvent(const ui::KeyEvent& key_event) override;
  bool IsPeekKeyEvent(const ui::KeyEvent& key_event) override;
  void SetCursorLocation(const gfx::Rect& rect) override;
  void Reset() override;
  void UpdateFocus(bool has_client,
                   ui::TextInputType old_type,
                   ui::TextInputType new_type) override;
  void SetSurroundingText(const std::u16string& text,
                          const gfx::Range& selection_range) override;
  void SetContentType(ui::TextInputType type,
                      ui::TextInputMode mode,
                      uint32_t flags,
                      bool should_do_learning) override;
  void SetGrammarFragmentAtCursor(
      const ui::GrammarFragment& fragment) override {}
  void SetAutocorrectInfo(const gfx::Range& autocorrect_range,
                          const gfx::Rect& autocorrect_bounds) override {}
  ui::VirtualKeyboardController* GetVirtualKeyboardController() override;

 private:
  // GtkIMContext event handlers.  They are shared among |gtk_context_simple_|
  // and |gtk_multicontext_|.
  CHROMEG_CALLBACK_1(InputMethodContextImplGtk,
                     void,
                     OnCommit,
                     GtkIMContext*,
                     gchar*);
  CHROMEG_CALLBACK_0(InputMethodContextImplGtk,
                     void,
                     OnPreeditChanged,
                     GtkIMContext*);
  CHROMEG_CALLBACK_0(InputMethodContextImplGtk,
                     void,
                     OnPreeditEnd,
                     GtkIMContext*);
  CHROMEG_CALLBACK_0(InputMethodContextImplGtk,
                     void,
                     OnPreeditStart,
                     GtkIMContext*);

  // Only used on GTK3.
  void SetContextClientWindow(GdkWindow* window, GtkIMContext* gtk_context);

  // Returns the IMContext depending on the currently connected input field
  // type.
  GtkIMContext* GetIMContext();

  // A set of callback functions.  Must not be nullptr.
  const raw_ptr<ui::LinuxInputMethodContextDelegate> delegate_;

  // Tracks the input field type.
  ui::TextInputType type_ = ui::TEXT_INPUT_TYPE_NONE;

  // IME's input GTK context.
  raw_ptr<GtkIMContext> gtk_context_ = nullptr;
  raw_ptr<GtkIMContext> gtk_simple_context_ = nullptr;

  // Only used on GTK3.
  gpointer gdk_last_set_client_window_ = nullptr;
  gpointer gdk_last_set_client_window_for_simple_ = nullptr;

  // Last known caret bounds relative to the screen coordinates, in DIPs.
  // Effective only on non-simple context.
  gfx::Rect last_caret_bounds_;
};

}  // namespace gtk

#endif  // UI_GTK_INPUT_METHOD_CONTEXT_IMPL_GTK_H_
