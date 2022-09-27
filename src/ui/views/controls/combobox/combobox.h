// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_H_
#define UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/combobox_model_observer.h"
#include "ui/base/models/menu_model.h"
#include "ui/color/color_id.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/prefix_delegate.h"
#include "ui/views/metadata/view_factory.h"
#include "ui/views/style/typography.h"

namespace gfx {
class FontList;
}

namespace ui {
class MenuModel;
}

namespace views {
namespace test {
class ComboboxTestApi;
}

class MenuRunner;
class PrefixSelector;

// A non-editable combobox (aka a drop-down list or selector).
// Combobox has two distinct parts, the drop down arrow and the text.
class VIEWS_EXPORT Combobox : public View,
                              public PrefixDelegate,
                              public ui::ComboboxModelObserver {
 public:
  METADATA_HEADER(Combobox);

  using MenuSelectionAtCallback = base::RepeatingCallback<bool(size_t index)>;

  static constexpr int kDefaultComboboxTextContext = style::CONTEXT_BUTTON;
  static constexpr int kDefaultComboboxTextStyle = style::STYLE_PRIMARY;

  // A combobox with an empty model.
  explicit Combobox(int text_context = kDefaultComboboxTextContext,
                    int text_style = kDefaultComboboxTextStyle);

  // |model| is owned by the combobox when using this constructor.
  explicit Combobox(std::unique_ptr<ui::ComboboxModel> model,
                    int text_context = kDefaultComboboxTextContext,
                    int text_style = kDefaultComboboxTextStyle);
  // |model| is not owned by the combobox when using this constructor.
  explicit Combobox(ui::ComboboxModel* model,
                    int text_context = kDefaultComboboxTextContext,
                    int text_style = kDefaultComboboxTextStyle);
  Combobox(const Combobox&) = delete;
  Combobox& operator=(const Combobox&) = delete;
  ~Combobox() override;

  const gfx::FontList& GetFontList() const;

  // Sets the callback which will be called when a selection has been made.
  void SetCallback(base::RepeatingClosure callback) {
    callback_ = std::move(callback);
  }

  // Set menu model.
  void SetMenuModel(std::unique_ptr<ui::MenuModel> menu_model) {
    menu_model_ = std::move(menu_model);
  }

  // Gets/Sets the selected index.
  absl::optional<size_t> GetSelectedIndex() const { return selected_index_; }
  void SetSelectedIndex(absl::optional<size_t> index);
  [[nodiscard]] base::CallbackListSubscription AddSelectedIndexChangedCallback(
      views::PropertyChangedCallback callback);

  // Called when there has been a selection from the menu.
  void MenuSelectionAt(size_t index);

  // Looks for the first occurrence of |value| in |model()|. If found, selects
  // the found index and returns true. Otherwise simply noops and returns false.
  bool SelectValue(const std::u16string& value);

  void SetOwnedModel(std::unique_ptr<ui::ComboboxModel> model);

  void SetModel(ui::ComboboxModel* model);
  ui::ComboboxModel* GetModel() const { return model_; }

  // Gets/Sets the tooltip text, and the accessible name if it is currently
  // empty.
  std::u16string GetTooltipTextAndAccessibleName() const;
  void SetTooltipTextAndAccessibleName(const std::u16string& tooltip_text);

  // Set the accessible name of the combobox.
  void SetAccessibleName(const std::u16string& name);
  std::u16string GetAccessibleName() const;

  // Visually marks the combobox as having an invalid value selected.
  // When invalid, it paints with white text on a red background.
  // Callers are responsible for restoring validity with selection changes.
  void SetInvalid(bool invalid);
  bool GetInvalid() const { return invalid_; }

  void SetBorderColorId(ui::ColorId color_id);
  void SetBackgroundColorId(ui::ColorId color_id);

  // Sets whether there should be ink drop highlighting on hover/press.
  void SetEventHighlighting(bool should_highlight);

  // Whether the combobox should use the largest label as the content size.
  void SetSizeToLargestLabel(bool size_to_largest_label);
  bool GetSizeToLargestLabel() const { return size_to_largest_label_; }

  void SetMenuSelectionAtCallback(MenuSelectionAtCallback callback) {
    menu_selection_at_callback_ = std::move(callback);
  }

  // Use the time when combobox was closed in order for parent view to not
  // treat a user event already treated by the combobox.
  base::TimeTicks GetClosedTime() { return closed_time_; }

  // Returns whether or not the menu is currently running.
  bool IsMenuRunning() const;

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  void OnBoundsChanged(const gfx::Rect& previous_bounds) override;
  bool SkipDefaultKeyEventProcessing(const ui::KeyEvent& e) override;
  bool OnKeyPressed(const ui::KeyEvent& e) override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;
  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;
  bool HandleAccessibleAction(const ui::AXActionData& action_data) override;
  void OnThemeChanged() override;

  // Overridden from PrefixDelegate:
  size_t GetRowCount() override;
  absl::optional<size_t> GetSelectedRow() override;
  void SetSelectedRow(absl::optional<size_t> row) override;
  std::u16string GetTextForRow(size_t row) override;

 protected:
  // Overridden from ComboboxModelObserver:
  void OnComboboxModelChanged(ui::ComboboxModel* model) override;
  void OnComboboxModelDestroying(ui::ComboboxModel* model) override;

  // Getters to be used by metadata.
  const base::RepeatingClosure& GetCallback() const;
  const std::unique_ptr<ui::ComboboxModel>& GetOwnedModel() const;

 private:
  friend class test::ComboboxTestApi;

  // Updates the border according to the current node_data.
  void UpdateBorder();

  // Given bounds within our View, this helper mirrors the bounds if necessary.
  void AdjustBoundsForRTLUI(gfx::Rect* rect) const;

  // Draws the selected value of the drop down list
  void PaintIconAndText(gfx::Canvas* canvas);

  // Opens the dropdown menu in response to |event|.
  void ArrowButtonPressed(const ui::Event& event);

  // Show the drop down list
  void ShowDropDownMenu(ui::MenuSourceType source_type);

  // Cleans up after the menu as closed
  void OnMenuClosed(Button::ButtonState original_button_state);

  // Called when the selection is changed by the user.
  void OnPerformAction();

  // Finds the size of the largest menu label.
  gfx::Size GetContentSize() const;

  // Returns the width needed to accommodate the provided width and checkmarks
  // and padding if checkmarks should be shown.
  int MaybeAdjustWidthForCheckmarks(int original_width) const;

  void OnContentSizeMaybeChanged();

  // Handles the clicking event.
  void HandleClickEvent();

  PrefixSelector* GetPrefixSelector();

  // Optionally used to tie the lifetime of the model to this combobox. See
  // constructor.
  std::unique_ptr<ui::ComboboxModel> owned_model_;

  // Reference to our model, which may be owned or not.
  raw_ptr<ui::ComboboxModel> model_ = nullptr;

  // Typography context for the text written in the combobox and the options
  // shown in the drop-down menu.
  const int text_context_;

  // Typography style for the text written in the combobox and the options shown
  // in the drop-down menu.
  const int text_style_;

  // Callback notified when the selected index changes.
  base::RepeatingClosure callback_;

  // Callback notified when the selected index is triggered to change. If set,
  // when a selection is made in the combobox this callback is called. If it
  // returns true no other action is taken, if it returns false then the model
  // will updated based on the selection.
  MenuSelectionAtCallback menu_selection_at_callback_;

  // The current selected index; nullopt means no selection.
  absl::optional<size_t> selected_index_ = absl::nullopt;

  // True when the selection is visually denoted as invalid.
  bool invalid_ = false;

  // True when there should be ink drop highlighting on hover and press.
  bool should_highlight_ = false;

  // Overriding ColorId for the combobox border.
  absl::optional<ui::ColorId> border_color_id_;

  // The accessible name of this combobox.
  std::u16string accessible_name_;

  // A helper used to select entries by keyboard input.
  std::unique_ptr<PrefixSelector> selector_;

  // The ComboboxModel for use by |menu_runner_|.
  std::unique_ptr<ui::MenuModel> menu_model_;

  // Like MenuButton, we use a time object in order to keep track of when the
  // combobox was closed. The time is used for simulating menu behavior; that
  // is, if the menu is shown and the button is pressed, we need to close the
  // menu. There is no clean way to get the second click event because the
  // menu is displayed using a modal loop and, unlike regular menus in Windows,
  // the button is not part of the displayed menu.
  base::TimeTicks closed_time_;

  // The maximum dimensions of the content in the dropdown.
  gfx::Size content_size_;

  // A transparent button that handles events and holds button state. Placed on
  // top of the combobox as a child view. Doesn't paint itself, but serves as a
  // host for inkdrops.
  raw_ptr<Button> arrow_button_;

  // Set while the dropdown is showing. Ensures the menu is closed if |this| is
  // destroyed.
  std::unique_ptr<MenuRunner> menu_runner_;

  // When true, the size of contents is defined by the widest label in the menu.
  // If this is set to true, the parent view must relayout in
  // ChildPreferredSizeChanged(). When false, the size of contents is defined by
  // the selected label
  bool size_to_largest_label_ = true;

  base::ScopedObservation<ui::ComboboxModel, ui::ComboboxModelObserver>
      observation_{this};
};

BEGIN_VIEW_BUILDER(VIEWS_EXPORT, Combobox, View)
VIEW_BUILDER_PROPERTY(base::RepeatingClosure, Callback)
VIEW_BUILDER_PROPERTY(std::unique_ptr<ui::ComboboxModel>, OwnedModel)
VIEW_BUILDER_PROPERTY(ui::ComboboxModel*, Model)
VIEW_BUILDER_PROPERTY(absl::optional<size_t>, SelectedIndex)
VIEW_BUILDER_PROPERTY(bool, Invalid)
VIEW_BUILDER_PROPERTY(bool, SizeToLargestLabel)
VIEW_BUILDER_PROPERTY(std::u16string, AccessibleName)
VIEW_BUILDER_PROPERTY(std::u16string, TooltipTextAndAccessibleName)
END_VIEW_BUILDER

}  // namespace views

DEFINE_VIEW_BUILDER(VIEWS_EXPORT, Combobox)

#endif  // UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_H_
