// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_display_host_manager.h"

#include <fcntl.h>
#include <stddef.h>
#include <xf86drm.h>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/events/ozone/device/device_event.h"
#include "ui/events/ozone/device/device_manager.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/host/drm_device_handle.h"
#include "ui/ozone/platform/drm/host/drm_display_host.h"
#include "ui/ozone/platform/drm/host/drm_native_display_delegate.h"
#include "ui/ozone/platform/drm/host/gpu_thread_adapter.h"
#include "ui/ozone/public/ozone_switches.h"

namespace ui {

namespace {

typedef base::OnceCallback<void(const base::FilePath&,
                                const base::FilePath&,
                                std::unique_ptr<DrmDeviceHandle>)>
    OnOpenDeviceReplyCallback;

const char kDefaultGraphicsCardPattern[] = "/dev/dri/card%d";

const char* kDisplayActionString[] = {
    "ADD",
    "REMOVE",
    "CHANGE",
};

// Find sysfs device path for the given device path.
base::FilePath MapDevPathToSysPath(const base::FilePath& device_path) {
  // |device_path| looks something like /dev/dri/card0. We take the basename of
  // that (card0) and append it to /sys/class/drm. /sys/class/drm/card0 is a
  // symlink that points to something like
  // /sys/devices/pci0000:00/0000:00:02.0/0000:05:00.0/drm/card0, which exposes
  // some metadata about the attached device.
  base::FilePath sys_path = base::MakeAbsoluteFilePath(
      base::FilePath("/sys/class/drm").Append(device_path.BaseName()));

  base::FilePath path_thus_far;
  for (const auto& component : sys_path.GetComponents()) {
    if (path_thus_far.empty()) {
      path_thus_far = base::FilePath(component);
    } else {
      path_thus_far = path_thus_far.Append(component);
    }

    // Newer versions of the EVDI kernel driver include a symlink to the USB
    // device in the sysfs EVDI directory (e.g.
    // /sys/devices/platform/evdi.0/device) for EVDI displays that are USB. If
    // that symlink exists, read it, and use that path as the sysfs path for the
    // display when calculating the association score to match it with a
    // corresponding USB touch device. If the symlink doesn't exist, use the
    // normal sysfs path. In order to ensure that the sysfs path remains unique,
    // append the card name to it.
    if (base::StartsWith(component, "evdi", base::CompareCase::SENSITIVE)) {
      base::FilePath usb_device_path;
      if (base::ReadSymbolicLink(path_thus_far.Append("device"),
                                 &usb_device_path)) {
        return base::MakeAbsoluteFilePath(path_thus_far.Append(usb_device_path))
            .Append(device_path.BaseName());
      }
      break;
    }
  }

  return sys_path;
}

void OpenDeviceAsync(const base::FilePath& device_path,
                     const scoped_refptr<base::TaskRunner>& reply_runner,
                     OnOpenDeviceReplyCallback callback) {
  base::FilePath sys_path = MapDevPathToSysPath(device_path);

  std::unique_ptr<DrmDeviceHandle> handle(new DrmDeviceHandle());
  handle->Initialize(device_path, sys_path);
  reply_runner->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), device_path, sys_path,
                                std::move(handle)));
}

struct DisplayCard {
  base::FilePath path;
  absl::optional<std::string> driver;
};

base::FilePath GetPrimaryDisplayCardPath() {
  struct drm_mode_card_res res;
  std::vector<DisplayCard> cards;
  for (int i = 0; /* end on first card# that does not exist */; i++) {
    std::string card_path = base::StringPrintf(kDefaultGraphicsCardPattern, i);

    if (access(card_path.c_str(), F_OK) != 0) {
      if (i == 0) /* card paths may start with 0 or 1 */
        continue;
      else
        break;
    }

    base::ScopedFD fd(open(card_path.c_str(), O_RDWR | O_CLOEXEC));
    if (!fd.is_valid()) {
      VPLOG(1) << "Failed to open '" << card_path << "'";
      continue;
    }

    memset(&res, 0, sizeof(struct drm_mode_card_res));
    int ret = drmIoctl(fd.get(), DRM_IOCTL_MODE_GETRESOURCES, &res);
    VPLOG_IF(1, ret) << "Failed to get DRM resources for '" << card_path << "'";

    if (ret == 0 && res.count_crtcs > 0)
      cards.push_back(
          {base::FilePath(card_path), GetDrmDriverNameFromFd(fd.get())});
  }

  // Find the card with the most preferred driver.
  const auto preferred_drivers = GetPreferredDrmDrivers();
  for (const auto* preferred_driver : preferred_drivers) {
    for (const auto& card : cards) {
      if (card.driver == preferred_driver)
        return card.path;
    }
  }

  // Fall back to the first usable card.
  if (!cards.empty())
    return cards[0].path;

  LOG(FATAL) << "Failed to open primary graphics device.";
  return base::FilePath();  // Not reached.
}

class FindDrmDisplayHostById {
 public:
  explicit FindDrmDisplayHostById(int64_t display_id)
      : display_id_(display_id) {}

  bool operator()(const std::unique_ptr<DrmDisplayHost>& display) const {
    return display->snapshot()->display_id() == display_id_;
  }

 private:
  int64_t display_id_;
};

}  // namespace

DrmDisplayHostManager::DrmDisplayHostManager(
    GpuThreadAdapter* proxy,
    DeviceManager* device_manager,
    OzonePlatform::PlatformRuntimeProperties* host_properties,
    InputControllerEvdev* input_controller)
    : proxy_(proxy),
      device_manager_(device_manager),
      input_controller_(input_controller),
      primary_graphics_card_path_(GetPrimaryDisplayCardPath()) {
  {
    // First device needs to be treated specially. We need to open this
    // synchronously since the GPU process will need it to initialize the
    // graphics state.
    base::ThreadRestrictions::ScopedAllowIO allow_io;

    base::FilePath primary_graphics_card_path_sysfs =
        MapDevPathToSysPath(primary_graphics_card_path_);

    primary_drm_device_handle_ = std::make_unique<DrmDeviceHandle>();
    if (!primary_drm_device_handle_->Initialize(
            primary_graphics_card_path_, primary_graphics_card_path_sysfs)) {
      LOG(FATAL) << "Failed to open primary graphics card";
      return;
    }
    host_properties->supports_overlays =
        primary_drm_device_handle_->has_atomic_capabilities();
    // TODO(b/192563524): The legacy video decoder wraps its frames with legacy
    // mailboxes instead of SharedImages. The display compositor can composite
    // these quads, but does not support promoting them to overlays. Thus, we
    // disable overlays on platforms using the legacy video decoder.
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(
            switches::kPlatformDisallowsChromeOSDirectVideoDecoder)) {
      host_properties->supports_overlays = false;
    }
    drm_devices_[primary_graphics_card_path_] =
        primary_graphics_card_path_sysfs;
  }

  device_manager_->AddObserver(this);
  proxy_->RegisterHandlerForDrmDisplayHostManager(this);
  proxy_->AddGpuThreadObserver(this);

  auto display_infos =
      GetAvailableDisplayControllerInfos(primary_drm_device_handle_->fd());
  has_dummy_display_ = !display_infos.empty();
  for (const auto& display_info : display_infos) {
    displays_.push_back(std::make_unique<DrmDisplayHost>(
        proxy_,
        CreateDisplaySnapshot(
            display_info.get(), primary_drm_device_handle_->fd(),
            primary_drm_device_handle_->sys_path(), 0, gfx::Point()),
        true /* is_dummy */));
  }
}

DrmDisplayHostManager::~DrmDisplayHostManager() {
  device_manager_->RemoveObserver(this);
  proxy_->UnRegisterHandlerForDrmDisplayHostManager();
  proxy_->RemoveGpuThreadObserver(this);
}

DrmDisplayHostManager::DisplayEvent::DisplayEvent(
    DeviceEvent::ActionType action_type,
    const base::FilePath& path,
    const EventPropertyMap& properties)
    : action_type(action_type), path(path), display_event_props(properties) {}

DrmDisplayHostManager::DisplayEvent::DisplayEvent(const DisplayEvent&) =
    default;
DrmDisplayHostManager::DisplayEvent&
DrmDisplayHostManager::DisplayEvent::operator=(const DisplayEvent&) = default;

DrmDisplayHostManager::DisplayEvent::~DisplayEvent() = default;

DrmDisplayHost* DrmDisplayHostManager::GetDisplay(int64_t display_id) {
  auto it = std::find_if(displays_.begin(), displays_.end(),
                         FindDrmDisplayHostById(display_id));
  if (it == displays_.end())
    return nullptr;

  return it->get();
}

void DrmDisplayHostManager::AddDelegate(DrmNativeDisplayDelegate* delegate) {
  DCHECK(!delegate_);
  delegate_ = delegate;
}

void DrmDisplayHostManager::RemoveDelegate(DrmNativeDisplayDelegate* delegate) {
  DCHECK_EQ(delegate_, delegate);
  delegate_ = nullptr;
}

void DrmDisplayHostManager::TakeDisplayControl(
    display::DisplayControlCallback callback) {
  if (display_control_change_pending_) {
    LOG(ERROR) << "TakeDisplayControl called while change already pending";
    std::move(callback).Run(false);
    return;
  }

  if (!display_externally_controlled_) {
    LOG(ERROR) << "TakeDisplayControl called while display already owned";
    std::move(callback).Run(true);
    return;
  }

  take_display_control_callback_ = std::move(callback);
  display_control_change_pending_ = true;

  if (!proxy_->GpuTakeDisplayControl())
    GpuTookDisplayControl(false);
}

void DrmDisplayHostManager::RelinquishDisplayControl(
    display::DisplayControlCallback callback) {
  if (display_control_change_pending_) {
    LOG(ERROR)
        << "RelinquishDisplayControl called while change already pending";
    std::move(callback).Run(false);
    return;
  }

  if (display_externally_controlled_) {
    LOG(ERROR) << "RelinquishDisplayControl called while display not owned";
    std::move(callback).Run(true);
    return;
  }

  relinquish_display_control_callback_ = std::move(callback);
  display_control_change_pending_ = true;

  if (!proxy_->GpuRelinquishDisplayControl())
    GpuRelinquishedDisplayControl(false);
}

void DrmDisplayHostManager::UpdateDisplays(
    display::GetDisplaysCallback callback) {
  get_displays_callback_ = std::move(callback);
  if (!proxy_->GpuRefreshNativeDisplays()) {
    RunUpdateDisplaysCallback(std::move(get_displays_callback_));
    get_displays_callback_.Reset();
  }
}

void DrmDisplayHostManager::ConfigureDisplays(
    const std::vector<display::DisplayConfigurationParams>& config_requests,
    display::ConfigureCallback callback,
    uint32_t modeset_flag) {
  for (auto& config : config_requests) {
    if (GetDisplay(config.id)->is_dummy()) {
      std::move(callback).Run(true);
      return;
    }
  }

  proxy_->GpuConfigureNativeDisplays(config_requests, std::move(callback),
                                     modeset_flag);
}

void DrmDisplayHostManager::OnDeviceEvent(const DeviceEvent& event) {
  if (event.device_type() != DeviceEvent::DISPLAY)
    return;

  event_queue_.push(
      DisplayEvent(event.action_type(), event.path(), event.properties()));
  ProcessEvent();
}

void DrmDisplayHostManager::ProcessEvent() {
  while (!event_queue_.empty() && !task_pending_) {
    DisplayEvent event = event_queue_.front();
    event_queue_.pop();
    auto seqnum_it = event.display_event_props.find("SEQNUM");
    const std::string seqnum = seqnum_it == event.display_event_props.end()
                                   ? ""
                                   : ("(SEQNUM:" + seqnum_it->second + ")");
    VLOG(1) << "Got display event " << kDisplayActionString[event.action_type]
            << seqnum << " for " << event.path.value();
    switch (event.action_type) {
      case DeviceEvent::ADD:
        if (drm_devices_.find(event.path) == drm_devices_.end()) {
          base::ThreadPool::PostTask(
              FROM_HERE,
              {base::MayBlock(),
               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
              base::BindOnce(
                  &OpenDeviceAsync, event.path,
                  base::ThreadTaskRunnerHandle::Get(),
                  base::BindOnce(&DrmDisplayHostManager::OnAddGraphicsDevice,
                                 weak_ptr_factory_.GetWeakPtr())));
          task_pending_ = true;
        }
        break;
      case DeviceEvent::CHANGE:
        task_pending_ = base::ThreadTaskRunnerHandle::Get()->PostTask(
            FROM_HERE,
            base::BindOnce(&DrmDisplayHostManager::OnUpdateGraphicsDevice,
                           weak_ptr_factory_.GetWeakPtr(),
                           event.display_event_props));
        break;
      case DeviceEvent::REMOVE:
        DCHECK(event.path != primary_graphics_card_path_)
            << "Removing primary graphics card";
        auto it = drm_devices_.find(event.path);
        if (it != drm_devices_.end()) {
          task_pending_ = base::ThreadTaskRunnerHandle::Get()->PostTask(
              FROM_HERE,
              base::BindOnce(&DrmDisplayHostManager::OnRemoveGraphicsDevice,
                             weak_ptr_factory_.GetWeakPtr(), it->second));
          drm_devices_.erase(it);
        }
        break;
    }
  }
}

void DrmDisplayHostManager::OnAddGraphicsDevice(
    const base::FilePath& dev_path,
    const base::FilePath& sys_path,
    std::unique_ptr<DrmDeviceHandle> handle) {
  if (handle->IsValid()) {
    drm_devices_[dev_path] = sys_path;
    proxy_->GpuAddGraphicsDevice(sys_path, handle->PassFD());
    NotifyDisplayDelegate();
  }

  task_pending_ = false;
  ProcessEvent();
}

void DrmDisplayHostManager::OnUpdateGraphicsDevice(
    const EventPropertyMap& udev_event_props) {
  proxy_->GpuShouldDisplayEventTriggerConfiguration(udev_event_props);
}

void DrmDisplayHostManager::OnRemoveGraphicsDevice(
    const base::FilePath& sys_path) {
  proxy_->GpuRemoveGraphicsDevice(sys_path);
  NotifyDisplayDelegate();
  task_pending_ = false;
  ProcessEvent();
}

void DrmDisplayHostManager::OnGpuProcessLaunched() {
  std::unique_ptr<DrmDeviceHandle> handle =
      std::move(primary_drm_device_handle_);
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;

    drm_devices_.clear();
    drm_devices_[primary_graphics_card_path_] =
        MapDevPathToSysPath(primary_graphics_card_path_);

    if (!handle) {
      handle = std::make_unique<DrmDeviceHandle>();
      if (!handle->Initialize(primary_graphics_card_path_,
                              drm_devices_[primary_graphics_card_path_]))
        LOG(FATAL) << "Failed to open primary graphics card";
    }
  }

  // Send the primary device first since this is used to initialize graphics
  // state.
  proxy_->GpuAddGraphicsDevice(drm_devices_[primary_graphics_card_path_],
                               handle->PassFD());
}

void DrmDisplayHostManager::OnGpuThreadReady() {
  // If in the middle of a configuration, just respond with the old list of
  // displays. This is fine, since after the DRM resources are initialized and
  // IPC-ed to the GPU NotifyDisplayDelegate() is called to let the display
  // delegate know that the display configuration changed and it needs to
  // update it again.
  if (!get_displays_callback_.is_null()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&DrmDisplayHostManager::RunUpdateDisplaysCallback,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(get_displays_callback_)));
    get_displays_callback_.Reset();
  }

  // Signal that we're taking DRM master since we're going through the
  // initialization process again and we'll take all the available resources.
  if (!take_display_control_callback_.is_null())
    GpuTookDisplayControl(true);

  if (!relinquish_display_control_callback_.is_null())
    GpuRelinquishedDisplayControl(false);

  device_manager_->ScanDevices(this);
  NotifyDisplayDelegate();
}

void DrmDisplayHostManager::OnGpuThreadRetired() {}

void DrmDisplayHostManager::GpuHasUpdatedNativeDisplays(
    MovableDisplaySnapshots displays) {
  if (delegate_)
    delegate_->OnDisplaySnapshotsInvalidated();
  std::vector<std::unique_ptr<DrmDisplayHost>> old_displays;
  displays_.swap(old_displays);
  for (auto& display : displays) {
    auto it = std::find_if(old_displays.begin(), old_displays.end(),
                           FindDrmDisplayHostById(display->display_id()));
    if (it == old_displays.end()) {
      displays_.push_back(std::make_unique<DrmDisplayHost>(
          proxy_, std::move(display), false /* is_dummy */));
    } else {
      (*it)->UpdateDisplaySnapshot(std::move(display));
      displays_.push_back(std::move(*it));
      old_displays.erase(it);
    }
  }

  if (!get_displays_callback_.is_null()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&DrmDisplayHostManager::RunUpdateDisplaysCallback,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(get_displays_callback_)));
    get_displays_callback_.Reset();
  }
}

void DrmDisplayHostManager::GpuReceivedHDCPState(
    int64_t display_id,
    bool status,
    display::HDCPState state,
    display::ContentProtectionMethod protection_method) {
  DrmDisplayHost* display = GetDisplay(display_id);
  if (display)
    display->OnHDCPStateReceived(status, state, protection_method);
  else
    LOG(ERROR) << "Couldn't find display with id=" << display_id;
}

void DrmDisplayHostManager::GpuUpdatedHDCPState(int64_t display_id,
                                                bool status) {
  DrmDisplayHost* display = GetDisplay(display_id);
  if (display)
    display->OnHDCPStateUpdated(status);
  else
    LOG(ERROR) << "Couldn't find display with id=" << display_id;
}

void DrmDisplayHostManager::GpuTookDisplayControl(bool status) {
  if (take_display_control_callback_.is_null()) {
    LOG(ERROR) << "No callback for take display control";
    return;
  }

  DCHECK(display_externally_controlled_);
  DCHECK(display_control_change_pending_);

  if (status) {
    input_controller_->SetInputDevicesEnabled(true);
    display_externally_controlled_ = false;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(take_display_control_callback_), status));
  take_display_control_callback_.Reset();
  display_control_change_pending_ = false;
}

void DrmDisplayHostManager::GpuRelinquishedDisplayControl(bool status) {
  if (relinquish_display_control_callback_.is_null()) {
    LOG(ERROR) << "No callback for relinquish display control";
    return;
  }

  DCHECK(!display_externally_controlled_);
  DCHECK(display_control_change_pending_);

  if (status) {
    input_controller_->SetInputDevicesEnabled(false);
    display_externally_controlled_ = true;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(relinquish_display_control_callback_), status));
  relinquish_display_control_callback_.Reset();
  display_control_change_pending_ = false;
}

void DrmDisplayHostManager::GpuShouldDisplayEventTriggerConfiguration(
    bool should_trigger) {
  if (should_trigger)
    NotifyDisplayDelegate();

  task_pending_ = false;
  ProcessEvent();
}

void DrmDisplayHostManager::RunUpdateDisplaysCallback(
    display::GetDisplaysCallback callback) const {
  std::vector<display::DisplaySnapshot*> snapshots;
  for (const auto& display : displays_)
    snapshots.push_back(display->snapshot());

  std::move(callback).Run(snapshots);
}

void DrmDisplayHostManager::NotifyDisplayDelegate() const {
  if (delegate_)
    delegate_->OnConfigurationChanged();
}

}  // namespace ui