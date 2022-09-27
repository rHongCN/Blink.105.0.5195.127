// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/color/color_provider_manager.h"

#include <algorithm>

#include "base/bind.h"
#include "base/check.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_provider_utils.h"

#if !BUILDFLAG(IS_ANDROID)
#include "ui/color/color_mixers.h"
#endif

namespace ui {

namespace {

// Cache at most 5 ColorProviders to prevent unbounded storage from user_color.
constexpr size_t kCacheSize = 5;

class GlobalManager : public ColorProviderManager {
 public:
  explicit GlobalManager(size_t cache_size = kCacheSize)
      : ColorProviderManager(cache_size) {}
  GlobalManager(const GlobalManager&) = delete;
  GlobalManager& operator=(const GlobalManager&) = delete;
  ~GlobalManager() override = default;
};

static_assert(sizeof(GlobalManager) == sizeof(ColorProviderManager),
              "Global manager is intended to provide constructor visibility to "
              "absl::optional, nothing more.");

absl::optional<GlobalManager>& GetGlobalManager() {
  static base::NoDestructor<absl::optional<GlobalManager>> manager;
  return *manager;
}

}  // namespace

ColorProviderManager::InitializerSupplier::InitializerSupplier() = default;

ColorProviderManager::InitializerSupplier::~InitializerSupplier() = default;

ColorProviderManager::ThemeInitializerSupplier::ThemeInitializerSupplier(
    ThemeType theme_type)
    : theme_type_(theme_type) {}

ColorProviderManager::Key::Key()
    : Key(ColorMode::kLight,
          ContrastMode::kNormal,
          SystemTheme::kDefault,
          FrameType::kChromium,
          absl::nullopt,
          nullptr) {}

ColorProviderManager::Key::Key(
    ColorMode color_mode,
    ContrastMode contrast_mode,
    SystemTheme system_theme,
    FrameType frame_type,
    absl::optional<SkColor> user_color,
    scoped_refptr<ThemeInitializerSupplier> custom_theme)
    : color_mode(color_mode),
      contrast_mode(contrast_mode),
      elevation_mode(ElevationMode::kLow),
      system_theme(system_theme),
      frame_type(frame_type),
      user_color(user_color),
      custom_theme(std::move(custom_theme)) {}

ColorProviderManager::Key::Key(const Key&) = default;

ColorProviderManager::Key& ColorProviderManager::Key::operator=(const Key&) =
    default;

ColorProviderManager::Key::~Key() = default;

ColorProviderManager::ColorProviderManager(size_t cache_size)
    : color_providers_(cache_size) {
  ResetColorProviderInitializerList();
}

ColorProviderManager::~ColorProviderManager() = default;

// static
ColorProviderManager& ColorProviderManager::Get() {
  absl::optional<GlobalManager>& manager = GetGlobalManager();
  if (!manager.has_value()) {
    manager.emplace();
#if !BUILDFLAG(IS_ANDROID)
    manager.value().AppendColorProviderInitializer(
        base::BindRepeating(AddColorMixers));
#endif  // !BUILDFLAG(IS_ANDROID)
  }

  return manager.value();
}

// static
ColorProviderManager& ColorProviderManager::GetForTesting(size_t cache_size) {
  absl::optional<GlobalManager>& manager = GetGlobalManager();
  if (!manager.has_value())
    manager.emplace(cache_size);
  return manager.value();
}

// static
void ColorProviderManager::ResetForTesting() {
  GetGlobalManager().reset();
}

void ColorProviderManager::ResetColorProviderInitializerList() {
  ResetColorProviderCache();
  initializer_list_ = std::make_unique<ColorProviderInitializerList>();
  initializer_subscriptions_.clear();
}

void ColorProviderManager::ResetColorProviderCache() {
  if (!color_providers_.empty())
    color_providers_.Clear();
}

void ColorProviderManager::AppendColorProviderInitializer(
    ColorProviderInitializerList::CallbackType initializer) {
  DCHECK(initializer_list_);
  ResetColorProviderCache();

  initializer_subscriptions_.push_back(
      initializer_list_->Add(std::move(initializer)));
}

ColorProvider* ColorProviderManager::GetColorProviderFor(Key key) {
  auto iter = color_providers_.Get(key);
  if (iter == color_providers_.end()) {
    auto provider = std::make_unique<ColorProvider>();
    DCHECK(initializer_list_);
    if (!initializer_list_->empty())
      initializer_list_->Notify(provider.get(), key);

    provider->GenerateColorMap();
    iter = color_providers_.Put(key, std::move(provider));
  }
  ColorProvider* provider = iter->second.get();
  DCHECK(provider);
  return provider;
}

}  // namespace ui
