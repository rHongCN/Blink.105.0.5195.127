// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/flatland/flatland_surface_factory.h"

#include <lib/sys/cpp/component_context.h>
#include <lib/zx/event.h>
#include <memory>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/fuchsia/fuchsia_logging.h"
#include "base/fuchsia/process_context.h"
#include "base/memory/ptr_util.h"
#include "third_party/angle/src/common/fuchsia_egl/fuchsia_egl.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/common/gl_ozone_egl.h"
#include "ui/ozone/platform/flatland/flatland_gpu_service.h"
#include "ui/ozone/platform/flatland/flatland_surface.h"
#include "ui/ozone/platform/flatland/flatland_sysmem_buffer_collection.h"
#include "ui/ozone/platform/flatland/flatland_window.h"
#include "ui/ozone/platform/flatland/flatland_window_manager.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "ui/ozone/platform/flatland/vulkan_implementation_flatland.h"
#endif

namespace ui {

namespace {

class GLOzoneEGLFlatland : public GLOzoneEGL {
 public:
  GLOzoneEGLFlatland() = default;
  ~GLOzoneEGLFlatland() override = default;
  GLOzoneEGLFlatland(const GLOzoneEGLFlatland&) = delete;
  GLOzoneEGLFlatland& operator=(const GLOzoneEGLFlatland&) = delete;

  // GLOzone:
  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override {
    // GL rendering to Flatland views is not supported. This function is
    // used only for unittests. Return an off-screen surface, so the tests pass.
    // TODO(crbug.com/1271760): Use Vulkan in unittests and remove this hack.
    return gl::InitializeGLSurface(base::MakeRefCounted<gl::SurfacelessEGL>(
        gl::GLSurfaceEGL::GetGLDisplayEGL(), gfx::Size(100, 100)));
  }

  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override {
    return gl::InitializeGLSurface(base::MakeRefCounted<gl::SurfacelessEGL>(
        gl::GLSurfaceEGL::GetGLDisplayEGL(), size));
  }

  gl::EGLDisplayPlatform GetNativeDisplay() override {
    return gl::EGLDisplayPlatform(EGL_DEFAULT_DISPLAY);
  }

 protected:
  bool LoadGLES2Bindings(
      const gl::GLImplementationParts& implementation) override {
    return LoadDefaultEGLGLES2Bindings(implementation);
  }
};

fuchsia::sysmem::AllocatorHandle ConnectSysmemAllocator() {
  fuchsia::sysmem::AllocatorHandle allocator;
  base::ComponentContextForProcess()->svc()->Connect(allocator.NewRequest());
  return allocator;
}

fuchsia::ui::composition::AllocatorHandle ConnectFlatlandAllocator() {
  fuchsia::ui::composition::AllocatorHandle allocator;
  base::ComponentContextForProcess()->svc()->Connect(allocator.NewRequest());
  return allocator;
}

}  // namespace

FlatlandSurfaceFactory::FlatlandSurfaceFactory()
    : egl_implementation_(std::make_unique<GLOzoneEGLFlatland>()),
      flatland_sysmem_buffer_manager_(this),
      weak_ptr_factory_(this) {}

FlatlandSurfaceFactory::~FlatlandSurfaceFactory() {
  Shutdown();
}

void FlatlandSurfaceFactory::Initialize(
    mojo::PendingRemote<mojom::ScenicGpuHost> gpu_host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::AutoLock lock(surface_lock_);
  DCHECK(surface_map_.empty());

  main_thread_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  DCHECK(main_thread_task_runner_);

  DCHECK(!gpu_host_);
  gpu_host_.Bind(std::move(gpu_host));

  flatland_sysmem_buffer_manager_.Initialize(ConnectSysmemAllocator(),
                                             ConnectFlatlandAllocator());
}

void FlatlandSurfaceFactory::Shutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::AutoLock lock(surface_lock_);
  DCHECK(surface_map_.empty());
  main_thread_task_runner_ = nullptr;
  gpu_host_.reset();
  flatland_sysmem_buffer_manager_.Shutdown();
}

std::vector<gl::GLImplementationParts>
FlatlandSurfaceFactory::GetAllowedGLImplementations() {
  return std::vector<gl::GLImplementationParts>{
      gl::GLImplementationParts(gl::kGLImplementationEGLANGLE),
      gl::GLImplementationParts(gl::ANGLEImplementation::kSwiftShader),
      gl::GLImplementationParts(gl::kGLImplementationEGLGLES2),
      gl::GLImplementationParts(gl::kGLImplementationStubGL),
  };
}

GLOzone* FlatlandSurfaceFactory::GetGLOzone(
    const gl::GLImplementationParts& implementation) {
  switch (implementation.gl) {
    case gl::kGLImplementationEGLGLES2:
    case gl::kGLImplementationEGLANGLE:
      return egl_implementation_.get();
    default:
      return nullptr;
  }
}

std::unique_ptr<PlatformWindowSurface>
FlatlandSurfaceFactory::CreatePlatformWindowSurface(
    gfx::AcceleratedWidget window) {
  DCHECK_NE(window, gfx::kNullAcceleratedWidget);
  auto surface = std::make_unique<FlatlandSurface>(this, window);
  main_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&FlatlandSurfaceFactory::AttachSurfaceToWindow,
                                weak_ptr_factory_.GetWeakPtr(), window,
                                surface->CreateView()));
  return surface;
}

std::unique_ptr<SurfaceOzoneCanvas>
FlatlandSurfaceFactory::CreateCanvasForWidget(gfx::AcceleratedWidget widget) {
  // TODO(fxbug.dev/93998): Add FlatlandWindowCanvas implementation.
  NOTREACHED();
  return nullptr;
}

scoped_refptr<gfx::NativePixmap> FlatlandSurfaceFactory::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    VkDevice vk_device,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    absl::optional<gfx::Size> framebuffer_size) {
  DCHECK(!framebuffer_size || framebuffer_size == size);

  auto collection = flatland_sysmem_buffer_manager_.CreateCollection(
      vk_device, size, format, usage, 1);
  if (!collection)
    return nullptr;

  return collection->CreateNativePixmap(0);
}

void FlatlandSurfaceFactory::CreateNativePixmapAsync(
    gfx::AcceleratedWidget widget,
    VkDevice vk_device,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    NativePixmapCallback callback) {
  std::move(callback).Run(
      CreateNativePixmap(widget, vk_device, size, format, usage));
}

scoped_refptr<gfx::NativePixmap>
FlatlandSurfaceFactory::CreateNativePixmapFromHandle(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::NativePixmapHandle handle) {
  auto collection = flatland_sysmem_buffer_manager_.GetCollectionById(
      handle.buffer_collection_id.value());
  if (!collection)
    return nullptr;

  return collection->CreateNativePixmap(handle.buffer_index);
}

#if BUILDFLAG(ENABLE_VULKAN)
std::unique_ptr<gpu::VulkanImplementation>
FlatlandSurfaceFactory::CreateVulkanImplementation(
    bool use_swiftshader,
    bool allow_protected_memory) {
  return std::make_unique<ui::VulkanImplementationFlatland>(
      this, &flatland_sysmem_buffer_manager_, use_swiftshader,
      allow_protected_memory);
}
#endif

void FlatlandSurfaceFactory::AddSurface(gfx::AcceleratedWidget widget,
                                        FlatlandSurface* surface) {
  base::AutoLock lock(surface_lock_);
  DCHECK(!base::Contains(surface_map_, widget));
  surface->AssertBelongsToCurrentThread();
  surface_map_.emplace(widget, surface);
}

void FlatlandSurfaceFactory::RemoveSurface(gfx::AcceleratedWidget widget) {
  base::AutoLock lock(surface_lock_);
  auto it = surface_map_.find(widget);
  DCHECK(it != surface_map_.end());
  FlatlandSurface* surface = it->second;
  surface->AssertBelongsToCurrentThread();
  surface_map_.erase(it);
}

FlatlandSurface* FlatlandSurfaceFactory::GetSurface(
    gfx::AcceleratedWidget widget) {
  base::AutoLock lock(surface_lock_);
  auto it = surface_map_.find(widget);
  if (it == surface_map_.end())
    return nullptr;

  FlatlandSurface* surface = it->second;
  surface->AssertBelongsToCurrentThread();
  return surface;
}

void FlatlandSurfaceFactory::AttachSurfaceToWindow(
    gfx::AcceleratedWidget window,
    mojo::PlatformHandle surface_view_holder_token_mojo) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  gpu_host_->AttachSurfaceToWindow(window,
                                   std::move(surface_view_holder_token_mojo));
}

}  // namespace ui
