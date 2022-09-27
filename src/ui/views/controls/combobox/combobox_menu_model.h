// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_MENU_MODEL_H_
#define UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_MENU_MODEL_H_

#include "base/i18n/rtl.h"
#include "ui/base/models/combobox_model.h"
#include "ui/base/models/image_model.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/menu/menu_config.h"

// Adapts a ui::ComboboxModel to a ui::MenuModel.
class VIEWS_EXPORT ComboboxMenuModel : public ui::MenuModel {
 public:
  ComboboxMenuModel(views::Combobox* owner, ui::ComboboxModel* model);
  ComboboxMenuModel(const ComboboxMenuModel&) = delete;
  ComboboxMenuModel& operator&(const ComboboxMenuModel&) = delete;
  ~ComboboxMenuModel() override;

 protected:
  ui::ComboboxModel* GetModel() const { return model_; }

 private:
  bool UseCheckmarks() const;

  // Overridden from MenuModel:
  bool HasIcons() const override;
  size_t GetItemCount() const override;
  ui::MenuModel::ItemType GetTypeAt(size_t index) const override;
  ui::MenuSeparatorType GetSeparatorTypeAt(size_t index) const override;
  int GetCommandIdAt(size_t index) const override;
  std::u16string GetLabelAt(size_t index) const override;
  std::u16string GetSecondaryLabelAt(size_t index) const override;
  bool IsItemDynamicAt(size_t index) const override;
  const gfx::FontList* GetLabelFontListAt(size_t index) const override;
  bool GetAcceleratorAt(size_t index,
                        ui::Accelerator* accelerator) const override;
  bool IsItemCheckedAt(size_t index) const override;
  int GetGroupIdAt(size_t index) const override;
  ui::ImageModel GetIconAt(size_t index) const override;
  ui::ButtonMenuItemModel* GetButtonMenuItemAt(size_t index) const override;
  bool IsEnabledAt(size_t index) const override;
  void ActivatedAt(size_t index) override;
  void ActivatedAt(size_t index, int event_flags) override;
  ui::MenuModel* GetSubmenuModelAt(size_t index) const override;

  raw_ptr<views::Combobox> owner_;    // Weak. Owns this.
  raw_ptr<ui::ComboboxModel> model_;  // Weak.
};

#endif  // UI_VIEWS_CONTROLS_COMBOBOX_COMBOBOX_MENU_MODEL_H_
