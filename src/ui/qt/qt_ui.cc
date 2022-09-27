// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/qt/qt_ui.h"

#include <dlfcn.h>

#include "base/check.h"
#include "base/command_line.h"
#include "base/cxx17_backports.h"
#include "base/memory/raw_ptr.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "chrome/browser/themes/theme_properties.h"  // nogncheck
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/font_render_params_linux.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/gfx/image/image_skia_source.h"
#include "ui/linux/linux_ui.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/native_theme/native_theme_base.h"
#include "ui/qt/qt_interface.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/shell_dialog_linux.h"
#include "ui/views/controls/button/label_button_border.h"

namespace qt {

namespace {

int QtWeightToCssWeight(int weight) {
  struct {
    int qt_weight;
    int css_weight;
  } constexpr kMapping[] = {
      // https://doc.qt.io/qt-5/qfont.html#Weight-enum
      {0, 100},  {12, 200}, {25, 300}, {50, 400}, {57, 500},
      {63, 600}, {75, 700}, {81, 800}, {87, 900}, {99, 1000},
  };

  weight = base::clamp(weight, 0, 99);
  for (size_t i = 0; i < std::size(kMapping) - 1; i++) {
    const auto& lo = kMapping[i];
    const auto& hi = kMapping[i + 1];
    if (weight <= hi.qt_weight) {
      return (weight - lo.qt_weight) * (hi.css_weight - lo.css_weight) /
                 (hi.qt_weight - lo.qt_weight) +
             lo.css_weight;
    }
  }
  NOTREACHED();
  return kMapping[std::size(kMapping) - 1].css_weight;
}

gfx::FontRenderParams::Hinting QtHintingToGfxHinting(
    qt::FontHinting hinting,
    gfx::FontRenderParams::Hinting default_hinting) {
  switch (hinting) {
    case FontHinting::kDefault:
      return default_hinting;
    case FontHinting::kNone:
      return gfx::FontRenderParams::HINTING_NONE;
    case FontHinting::kLight:
      return gfx::FontRenderParams::HINTING_SLIGHT;
    case FontHinting::kFull:
      return gfx::FontRenderParams::HINTING_FULL;
  }
}

}  // namespace

class QtNativeTheme : public ui::NativeThemeAura {
 public:
  explicit QtNativeTheme(QtInterface* shim)
      : ui::NativeThemeAura(/*use_overlay_scrollbars=*/false,
                            /*should_only_use_dark_colors=*/false,
                            /*is_custom_system_theme=*/true),
        shim_(shim) {}
  QtNativeTheme(const QtNativeTheme&) = delete;
  QtNativeTheme& operator=(const QtNativeTheme&) = delete;
  ~QtNativeTheme() override = default;

  // ui::NativeTheme:
  void PaintFrameTopArea(cc::PaintCanvas* canvas,
                         State state,
                         const gfx::Rect& rect,
                         const FrameTopAreaExtraParams& frame_top_area,
                         ColorScheme color_scheme) const override {
    auto image = shim_->DrawHeader(
        rect.width(), rect.height(), frame_top_area.default_background_color,
        frame_top_area.is_active ? ColorState::kNormal : ColorState::kInactive,
        frame_top_area.use_custom_frame);
    SkImageInfo image_info = SkImageInfo::Make(
        image.width, image.height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    SkBitmap bitmap;
    bitmap.installPixels(
        image_info, image.data_argb.Take(), image_info.minRowBytes(),
        [](void* data, void*) { free(data); }, nullptr);
    bitmap.setImmutable();
    canvas->drawImage(cc::PaintImage::CreateFromBitmap(std::move(bitmap)),
                      rect.x(), rect.y());
  }

 private:
  raw_ptr<QtInterface> const shim_;
};

QtUi::QtUi(std::unique_ptr<ui::LinuxUi> fallback_linux_ui)
    : fallback_linux_ui_(std::move(fallback_linux_ui)) {}

QtUi::~QtUi() {
  shell_dialog_linux::Finalize();
}

std::unique_ptr<ui::LinuxInputMethodContext> QtUi::CreateInputMethodContext(
    ui::LinuxInputMethodContextDelegate* delegate) const {
  return fallback_linux_ui_
             ? fallback_linux_ui_->CreateInputMethodContext(delegate)
             : nullptr;
}

gfx::FontRenderParams QtUi::GetDefaultFontRenderParams() const {
  return font_params_;
}

void QtUi::GetDefaultFontDescription(std::string* family_out,
                                     int* size_pixels_out,
                                     int* style_out,
                                     int* weight_out,
                                     gfx::FontRenderParams* params_out) const {
  if (family_out)
    *family_out = font_family_;
  if (size_pixels_out)
    *size_pixels_out = font_size_pixels_;
  if (style_out)
    *style_out = font_style_;
  if (weight_out)
    *weight_out = font_weight_;
  if (params_out)
    *params_out = font_params_;
}

ui::SelectFileDialog* QtUi::CreateSelectFileDialog(
    void* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy) const {
  return fallback_linux_ui_ ? fallback_linux_ui_->CreateSelectFileDialog(
                                  listener, std::move(policy))
                            : nullptr;
}

bool QtUi::Initialize() {
  base::FilePath path;
  if (!base::PathService::Get(base::DIR_MODULE, &path))
    return false;
  path = path.Append("libqt5_shim.so");
  void* libqt_shim = dlopen(path.value().c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!libqt_shim)
    return false;
  void* create_qt_interface = dlsym(libqt_shim, "CreateQtInterface");
  DCHECK(create_qt_interface);

  cmd_line_ = CopyCmdLine(*base::CommandLine::ForCurrentProcess());
  shim_.reset((reinterpret_cast<decltype(&CreateQtInterface)>(
      create_qt_interface)(this, &cmd_line_.argc, cmd_line_.argv.data())));
  native_theme_ = std::make_unique<QtNativeTheme>(shim_.get());
  ui::ColorProviderManager::Get().AppendColorProviderInitializer(
      base::BindRepeating(&QtUi::AddNativeColorMixer, base::Unretained(this)));
  FontChanged();
  shell_dialog_linux::Initialize();

  return true;
}

bool QtUi::GetColor(int id, SkColor* color, bool use_custom_frame) const {
  auto value = GetColor(id, use_custom_frame);
  if (value)
    *color = *value;
  return value.has_value();
}

bool QtUi::GetDisplayProperty(int id, int* result) const {
  switch (id) {
    case ThemeProperties::SHOULD_FILL_BACKGROUND_TAB_COLOR:
      *result = false;
      return true;
    default:
      return false;
  }
}

SkColor QtUi::GetFocusRingColor() const {
  return shim_->GetColor(ColorType::kHighlightBg, ColorState::kNormal);
}

SkColor QtUi::GetActiveSelectionBgColor() const {
  return shim_->GetColor(ColorType::kHighlightBg, ColorState::kNormal);
}

SkColor QtUi::GetActiveSelectionFgColor() const {
  return shim_->GetColor(ColorType::kHighlightFg, ColorState::kNormal);
}

SkColor QtUi::GetInactiveSelectionBgColor() const {
  return shim_->GetColor(ColorType::kHighlightBg, ColorState::kInactive);
}

SkColor QtUi::GetInactiveSelectionFgColor() const {
  return shim_->GetColor(ColorType::kHighlightFg, ColorState::kInactive);
}

base::TimeDelta QtUi::GetCursorBlinkInterval() const {
  return base::Milliseconds(shim_->GetCursorBlinkIntervalMs());
}

gfx::Image QtUi::GetIconForContentType(const std::string& content_type,
                                       int size,
                                       float scale) const {
  Image image =
      shim_->GetIconForContentType(String(content_type.c_str()), size * scale);
  if (!image.data_argb.size())
    return {};

  SkImageInfo image_info = SkImageInfo::Make(
      image.width, image.height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  SkBitmap bitmap;
  bitmap.installPixels(
      image_info, image.data_argb.Take(), image_info.minRowBytes(),
      [](void* data, void*) { free(data); }, nullptr);
  gfx::ImageSkia image_skia =
      gfx::ImageSkia::CreateFromBitmap(bitmap, image.scale);
  image_skia.MakeThreadSafe();
  return gfx::Image(image_skia);
}

QtUi::WindowFrameAction QtUi::GetWindowFrameAction(
    WindowFrameActionSource source) {
  // QT doesn't have settings for the window frame action since it prefers
  // server-side decorations.  So use the hardcoded behavior of a QMdiSubWindow,
  // which also matches the default Chrome behavior when there's no LinuxUI.
  switch (source) {
    case WindowFrameActionSource::kDoubleClick:
      return WindowFrameAction::kToggleMaximize;
    case WindowFrameActionSource::kMiddleClick:
      return WindowFrameAction::kNone;
    case WindowFrameActionSource::kRightClick:
      return WindowFrameAction::kMenu;
  }
}

float QtUi::GetDeviceScaleFactor() const {
  return shim_->GetScaleFactor();
}

bool QtUi::PreferDarkTheme() const {
  return color_utils::IsDark(
      shim_->GetColor(ColorType::kWindowBg, ColorState::kNormal));
}

bool QtUi::AnimationsEnabled() const {
  return shim_->GetAnimationDurationMs() > 0;
}

std::unique_ptr<ui::NavButtonProvider> QtUi::CreateNavButtonProvider() {
  // QT prefers server-side decorations.
  return nullptr;
}

ui::WindowFrameProvider* QtUi::GetWindowFrameProvider(bool solid_frame) {
  // QT prefers server-side decorations.
  return nullptr;
}

base::flat_map<std::string, std::string> QtUi::GetKeyboardLayoutMap() {
  return fallback_linux_ui_ ? fallback_linux_ui_->GetKeyboardLayoutMap()
                            : base::flat_map<std::string, std::string>{};
}

std::string QtUi::GetCursorThemeName() {
  // This is only used on X11 where QT obtains the cursor theme from XSettings.
  // However, ui/base/x/x11_cursor_loader.cc already handles this.
  return std::string();
}

int QtUi::GetCursorThemeSize() {
  // This is only used on X11 where QT obtains the cursor size from XSettings.
  // However, ui/base/x/x11_cursor_loader.cc already handles this.
  return 0;
}

ui::NativeTheme* QtUi::GetNativeThemeImpl() const {
  return native_theme_.get();
}

bool QtUi::GetTextEditCommandsForEvent(
    const ui::Event& event,
    std::vector<ui::TextEditCommandAuraLinux>* commands) {
  // QT doesn't have "key themes" (eg. readline bindings) like GTK.
  return false;
}

#if BUILDFLAG(ENABLE_PRINTING)
printing::PrintDialogLinuxInterface* QtUi::CreatePrintDialog(
    printing::PrintingContextLinux* context) {
  return fallback_linux_ui_ ? fallback_linux_ui_->CreatePrintDialog(context)
                            : nullptr;
}

gfx::Size QtUi::GetPdfPaperSize(printing::PrintingContextLinux* context) {
  return fallback_linux_ui_ ? fallback_linux_ui_->GetPdfPaperSize(context)
                            : gfx::Size();
}
#endif

void QtUi::FontChanged() {
  auto params = shim_->GetFontRenderParams();
  auto desc = shim_->GetFontDescription();

  font_family_ = desc.family.c_str();
  if (desc.size_pixels > 0) {
    font_size_pixels_ = desc.size_pixels;
    font_size_points_ = font_size_pixels_ / GetDeviceScaleFactor();
  } else {
    font_size_points_ = desc.size_points;
    font_size_pixels_ = font_size_points_ * GetDeviceScaleFactor();
  }
  font_style_ = desc.is_italic ? gfx::Font::ITALIC : gfx::Font::NORMAL;
  font_weight_ = QtWeightToCssWeight(desc.weight);

  gfx::FontRenderParamsQuery query;
  query.families = {font_family_};
  query.pixel_size = font_size_pixels_;
  query.point_size = font_size_points_;
  query.style = font_style_;
  query.weight = static_cast<gfx::Font::Weight>(font_weight_);

  gfx::FontRenderParams fc_params;
  gfx::QueryFontconfig(query, &fc_params, nullptr);
  font_params_ = gfx::FontRenderParams{
      .antialiasing = params.antialiasing,
      .use_bitmaps = params.use_bitmaps,
      .hinting = QtHintingToGfxHinting(params.hinting, fc_params.hinting),
      // QT doesn't expose a subpixel rendering setting, so fall back to
      // fontconfig for it.
      .subpixel_rendering = fc_params.subpixel_rendering,
  };
}

void QtUi::ThemeChanged() {
  native_theme_->NotifyOnNativeThemeUpdated();
}

void QtUi::AddNativeColorMixer(ui::ColorProvider* provider,
                               const ui::ColorProviderManager::Key& key) {
  if (key.system_theme == ui::ColorProviderManager::SystemTheme::kDefault)
    return;

  ui::ColorMixer& mixer = provider->AddMixer();
  // These color constants are required by native_chrome_color_mixer_linux.cc
  struct {
    ui::ColorId id;
    ColorType role;
    ColorState state = ColorState::kNormal;
  } const kMaps[] = {
      // Core colors
      {ui::kColorAccent, ColorType::kHighlightBg},
      {ui::kColorDisabledForeground, ColorType::kWindowFg,
       ColorState::kDisabled},
      {ui::kColorEndpointBackground, ColorType::kEntryBg},
      {ui::kColorEndpointForeground, ColorType::kEntryFg},
      {ui::kColorItemHighlight, ColorType::kHighlightBg},
      {ui::kColorItemSelectionBackground, ColorType::kHighlightBg},
      {ui::kColorMenuSelectionBackground, ColorType::kHighlightBg},
      {ui::kColorMidground, ColorType::kMidground},
      {ui::kColorPrimaryBackground, ColorType::kWindowBg},
      {ui::kColorPrimaryForeground, ColorType::kWindowFg},
      {ui::kColorSecondaryForeground, ColorType::kWindowFg,
       ColorState::kDisabled},
      {ui::kColorSubtleAccent, ColorType::kHighlightBg, ColorState::kInactive},
      {ui::kColorSubtleEmphasisBackground, ColorType::kWindowBg},
      {ui::kColorTextSelectionBackground, ColorType::kHighlightBg},
      {ui::kColorTextSelectionForeground, ColorType::kHighlightFg},

      // UI element colors
      {ui::kColorMenuBackground, ColorType::kEntryBg},
      {ui::kColorMenuItemBackgroundHighlighted, ColorType::kHighlightBg},
      {ui::kColorMenuItemBackgroundSelected, ColorType::kHighlightBg},
      {ui::kColorMenuItemForeground, ColorType::kEntryFg},
      {ui::kColorMenuItemForegroundHighlighted, ColorType::kHighlightFg},
      {ui::kColorMenuItemForegroundSelected, ColorType::kHighlightFg},

      // Platform-specific UI elements
      {ui::kColorNativeButtonBorder,
       // For flat-styled buttons, QT uses the text color as the button border.
       ColorType::kWindowFg},
      {ui::kColorNativeHeaderButtonBorderActive, ColorType::kWindowFg},
      {ui::kColorNativeHeaderButtonBorderInactive, ColorType::kWindowFg,
       ColorState::kInactive},
      {ui::kColorNativeHeaderSeparatorBorderActive, ColorType::kWindowFg},
      {ui::kColorNativeHeaderSeparatorBorderInactive, ColorType::kWindowFg,
       ColorState::kInactive},
      {ui::kColorNativeLabelForeground, ColorType::kWindowFg},
      {ui::kColorNativeTabForegroundInactiveFrameActive, ColorType::kButtonFg},
      {ui::kColorNativeTabForegroundInactiveFrameInactive, ColorType::kButtonFg,
       ColorState::kInactive},
      {ui::kColorNativeTextfieldBorderUnfocused, ColorType::kWindowFg,
       ColorState::kInactive},
      {ui::kColorNativeToolbarBackground, ColorType::kButtonBg},
  };
  for (const auto& map : kMaps)
    mixer[map.id] = {shim_->GetColor(map.role, map.state)};

  mixer[ui::kColorFrameActive] = {
      shim_->GetFrameColor(ColorState::kNormal, true)};
  mixer[ui::kColorFrameInactive] = {
      shim_->GetFrameColor(ColorState::kInactive, true)};
}

absl::optional<SkColor> QtUi::GetColor(int id, bool use_custom_frame) const {
  switch (id) {
    case ThemeProperties::COLOR_LOCATION_BAR_BORDER:
      return shim_->GetColor(ColorType::kEntryFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_CONTENT_AREA_SEPARATOR:
      return shim_->GetColor(ColorType::kButtonFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_VERTICAL_SEPARATOR:
      return shim_->GetColor(ColorType::kButtonFg, ColorState::kNormal);
    case ThemeProperties::COLOR_NTP_BACKGROUND:
      return shim_->GetColor(ColorType::kEntryBg, ColorState::kNormal);
    case ThemeProperties::COLOR_NTP_TEXT:
      return shim_->GetColor(ColorType::kEntryFg, ColorState::kNormal);
    case ThemeProperties::COLOR_NTP_HEADER:
      return shim_->GetColor(ColorType::kButtonFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return shim_->GetColor(ColorType::kWindowFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_HOVERED:
      return shim_->GetColor(ColorType::kWindowFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON_PRESSED:
      return shim_->GetColor(ColorType::kWindowFg, ColorState::kNormal);
    case ThemeProperties::COLOR_TOOLBAR_TEXT:
      return shim_->GetColor(ColorType::kWindowFg, ColorState::kNormal);
    case ThemeProperties::COLOR_NTP_LINK:
      return shim_->GetColor(ColorType::kHighlightBg, ColorState::kNormal);
    case ThemeProperties::COLOR_FRAME_ACTIVE:
      return shim_->GetFrameColor(ColorState::kNormal, use_custom_frame);
    case ThemeProperties::COLOR_FRAME_INACTIVE:
      return shim_->GetFrameColor(ColorState::kInactive, use_custom_frame);
    case ThemeProperties::COLOR_FRAME_ACTIVE_INCOGNITO:
      return shim_->GetFrameColor(ColorState::kNormal, use_custom_frame);
    case ThemeProperties::COLOR_FRAME_INACTIVE_INCOGNITO:
      return shim_->GetFrameColor(ColorState::kInactive, use_custom_frame);
    case ThemeProperties::COLOR_TOOLBAR:
      return shim_->GetColor(ColorType::kButtonBg, ColorState::kNormal);
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_ACTIVE:
      return shim_->GetColor(ColorType::kButtonBg, ColorState::kNormal);
    case ThemeProperties::COLOR_TAB_BACKGROUND_ACTIVE_FRAME_INACTIVE:
      return shim_->GetColor(ColorType::kButtonBg, ColorState::kInactive);
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_ACTIVE:
      return color_utils::BlendForMinContrast(
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kNormal),
                 shim_->GetFrameColor(ColorState::kNormal, use_custom_frame))
          .color;
    case ThemeProperties::COLOR_TAB_FOREGROUND_INACTIVE_FRAME_INACTIVE:
      return color_utils::BlendForMinContrast(
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kInactive),
                 shim_->GetFrameColor(ColorState::kInactive, use_custom_frame))
          .color;
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_ACTIVE:
      return color_utils::BlendForMinContrast(
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kNormal),
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kNormal),
                 SK_ColorBLACK, 2.0)
          .color;
    case ThemeProperties::COLOR_TAB_STROKE_FRAME_INACTIVE:
      return color_utils::BlendForMinContrast(
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kInactive),
                 shim_->GetColor(ColorType::kButtonBg, ColorState::kInactive),
                 SK_ColorBLACK, 2.0)
          .color;
    default:
      return absl::nullopt;
  }
}

std::unique_ptr<ui::LinuxUi> CreateQtUi(
    std::unique_ptr<ui::LinuxUi> fallback_linux_ui) {
  return std::make_unique<QtUi>(std::move(fallback_linux_ui));
}

}  // namespace qt
