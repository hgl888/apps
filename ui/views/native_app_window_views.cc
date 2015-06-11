// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/ui/views/native_app_window_views.h"

#include "apps/app_window.h"
#include "base/threading/sequenced_worker_pool.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "extensions/common/draggable_region.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/gfx/path.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace apps {

NativeAppWindowViews::NativeAppWindowViews()
    : app_window_(NULL),
      web_view_(NULL),
      window_(NULL),
      frameless_(false),
      transparent_background_(false),
      resizable_(false) {}

void NativeAppWindowViews::Init(AppWindow* app_window,
                                const AppWindow::CreateParams& create_params) {
  app_window_ = app_window;
  frameless_ = create_params.frame == AppWindow::FRAME_NONE;
  transparent_background_ = create_params.transparent_background;
  resizable_ = create_params.resizable;
  Observe(app_window_->web_contents());

  window_ = new views::Widget;
  InitializeWindow(app_window, create_params);

  OnViewWasResized();
  window_->AddObserver(this);
}

NativeAppWindowViews::~NativeAppWindowViews() {
  web_view_->SetWebContents(NULL);
}

void NativeAppWindowViews::InitializeWindow(
    AppWindow* app_window,
    const AppWindow::CreateParams& create_params) {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  views::Widget::InitParams init_params(views::Widget::InitParams::TYPE_WINDOW);
  init_params.delegate = this;
  init_params.top_level = true;
  init_params.keep_on_top = create_params.always_on_top;
  window_->Init(init_params);
  window_->CenterWindow(
      create_params.GetInitialWindowBounds(gfx::Insets()).size());
}

// ui::BaseWindow implementation.

bool NativeAppWindowViews::IsActive() const {
  return window_->IsActive();
}

bool NativeAppWindowViews::IsMaximized() const {
  return window_->IsMaximized();
}

bool NativeAppWindowViews::IsMinimized() const {
  return window_->IsMinimized();
}

bool NativeAppWindowViews::IsFullscreen() const {
  return window_->IsFullscreen();
}

gfx::NativeWindow NativeAppWindowViews::GetNativeWindow() {
  return window_->GetNativeWindow();
}

gfx::Rect NativeAppWindowViews::GetRestoredBounds() const {
  return window_->GetRestoredBounds();
}

ui::WindowShowState NativeAppWindowViews::GetRestoredState() const {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  if (IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;
  return ui::SHOW_STATE_NORMAL;
}

gfx::Rect NativeAppWindowViews::GetBounds() const {
  return window_->GetWindowBoundsInScreen();
}

void NativeAppWindowViews::Show() {
  if (window_->IsVisible()) {
    window_->Activate();
    return;
  }
  window_->Show();
}

void NativeAppWindowViews::ShowInactive() {
  if (window_->IsVisible())
    return;

  window_->ShowInactive();
}

void NativeAppWindowViews::Hide() {
  window_->Hide();
}

void NativeAppWindowViews::Close() {
  window_->Close();
}

void NativeAppWindowViews::Activate() {
  window_->Activate();
}

void NativeAppWindowViews::Deactivate() {
  window_->Deactivate();
}

void NativeAppWindowViews::Maximize() {
  window_->Maximize();
}

void NativeAppWindowViews::Minimize() {
  window_->Minimize();
}

void NativeAppWindowViews::Restore() {
  window_->Restore();
}

void NativeAppWindowViews::SetBounds(const gfx::Rect& bounds) {
  window_->SetBounds(bounds);
}

void NativeAppWindowViews::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

bool NativeAppWindowViews::IsAlwaysOnTop() const {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  return window_->IsAlwaysOnTop();
}

void NativeAppWindowViews::SetAlwaysOnTop(bool always_on_top) {
  window_->SetAlwaysOnTop(always_on_top);
}

gfx::NativeView NativeAppWindowViews::GetHostView() const {
  return window_->GetNativeView();
}

gfx::Point NativeAppWindowViews::GetDialogPosition(const gfx::Size& size) {
  gfx::Size app_window_size = window_->GetWindowBoundsInScreen().size();
  return gfx::Point(app_window_size.width() / 2 - size.width() / 2,
                    app_window_size.height() / 2 - size.height() / 2);
}

gfx::Size NativeAppWindowViews::GetMaximumDialogSize() {
  return window_->GetWindowBoundsInScreen().size();
}

void NativeAppWindowViews::AddObserver(
    web_modal::ModalDialogHostObserver* observer) {
  observer_list_.AddObserver(observer);
}
void NativeAppWindowViews::RemoveObserver(
    web_modal::ModalDialogHostObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void NativeAppWindowViews::OnViewWasResized() {
  FOR_EACH_OBSERVER(web_modal::ModalDialogHostObserver,
                    observer_list_,
                    OnPositionRequiresUpdate());
}

// WidgetDelegate implementation.

void NativeAppWindowViews::OnWidgetMove() {
  app_window_->OnNativeWindowChanged();
}

views::View* NativeAppWindowViews::GetInitiallyFocusedView() {
  return web_view_;
}

bool NativeAppWindowViews::CanResize() const {
  return resizable_ && !size_constraints_.HasFixedSize();
}

bool NativeAppWindowViews::CanMaximize() const {
  return resizable_ && !size_constraints_.HasMaximumSize() &&
         !app_window_->window_type_is_panel();
}

base::string16 NativeAppWindowViews::GetWindowTitle() const {
  return app_window_->GetTitle();
}

bool NativeAppWindowViews::ShouldShowWindowTitle() const {
  return app_window_->window_type() == AppWindow::WINDOW_TYPE_V1_PANEL;
}

bool NativeAppWindowViews::ShouldShowWindowIcon() const {
  return app_window_->window_type() == AppWindow::WINDOW_TYPE_V1_PANEL;
}

void NativeAppWindowViews::SaveWindowPlacement(const gfx::Rect& bounds,
                                               ui::WindowShowState show_state) {
  views::WidgetDelegate::SaveWindowPlacement(bounds, show_state);
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowViews::DeleteDelegate() {
  window_->RemoveObserver(this);
  app_window_->OnNativeClose();
}

views::Widget* NativeAppWindowViews::GetWidget() {
  return window_;
}

const views::Widget* NativeAppWindowViews::GetWidget() const {
  return window_;
}

views::View* NativeAppWindowViews::GetContentsView() {
  return this;
}

bool NativeAppWindowViews::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
#if defined(USE_AURA)
  if (child->Contains(web_view_->web_contents()->GetView()->GetNativeView())) {
    // App window should claim mouse events that fall within the draggable
    // region.
    return !draggable_region_.get() ||
           !draggable_region_->contains(location.x(), location.y());
  }
#endif

  return true;
}

// WidgetObserver implementation.

void NativeAppWindowViews::OnWidgetVisibilityChanged(views::Widget* widget,
                                                     bool visible) {
  app_window_->OnNativeWindowChanged();
}

void NativeAppWindowViews::OnWidgetActivationChanged(views::Widget* widget,
                                                     bool active) {
  app_window_->OnNativeWindowChanged();
  if (active)
    app_window_->OnNativeWindowActivated();
}

// WebContentsObserver implementation.

void NativeAppWindowViews::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  if (transparent_background_) {
    // Use a background with transparency to trigger transparency in Webkit.
    SkBitmap background;
    background.setConfig(SkBitmap::kARGB_8888_Config, 1, 1);
    background.allocPixels();
    background.eraseARGB(0x00, 0x00, 0x00, 0x00);

    content::RenderWidgetHostView* view = render_view_host->GetView();
    DCHECK(view);
    view->SetBackground(background);
  }
}

void NativeAppWindowViews::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  OnViewWasResized();
}

// views::View implementation.

void NativeAppWindowViews::Layout() {
  DCHECK(web_view_);
  web_view_->SetBounds(0, 0, width(), height());
  OnViewWasResized();
}

void NativeAppWindowViews::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this) {
    web_view_ = new views::WebView(NULL);
    AddChildView(web_view_);
    web_view_->SetWebContents(app_window_->web_contents());
  }
}

gfx::Size NativeAppWindowViews::GetMinimumSize() {
  return size_constraints_.GetMinimumSize();
}

gfx::Size NativeAppWindowViews::GetMaximumSize() {
  return size_constraints_.GetMaximumSize();
}

void NativeAppWindowViews::OnFocus() {
  web_view_->RequestFocus();
}

// NativeAppWindow implementation.

void NativeAppWindowViews::SetFullscreen(int fullscreen_types) {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  window_->SetFullscreen(fullscreen_types != AppWindow::FULLSCREEN_TYPE_NONE);
}

bool NativeAppWindowViews::IsFullscreenOrPending() const {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  return window_->IsFullscreen();
}

bool NativeAppWindowViews::IsDetached() const {
  // Stub implementation. See also ChromeNativeAppWindowViews.
  return false;
}

void NativeAppWindowViews::UpdateWindowIcon() {
  window_->UpdateWindowIcon();
}

void NativeAppWindowViews::UpdateWindowTitle() {
  window_->UpdateWindowTitle();
}

void NativeAppWindowViews::UpdateBadgeIcon() {
  // Stub implementation. See also ChromeNativeAppWindowViews.
}

void NativeAppWindowViews::UpdateDraggableRegions(
    const std::vector<extensions::DraggableRegion>& regions) {
  // Draggable region is not supported for non-frameless window.
  if (!frameless_)
    return;

  draggable_region_.reset(AppWindow::RawDraggableRegionsToSkRegion(regions));
  OnViewWasResized();
}

SkRegion* NativeAppWindowViews::GetDraggableRegion() {
  return draggable_region_.get();
}

void NativeAppWindowViews::UpdateShape(scoped_ptr<SkRegion> region) {
  // Stub implementation. See also ChromeNativeAppWindowViews.
}

void NativeAppWindowViews::HandleKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  unhandled_keyboard_event_handler_.HandleKeyboardEvent(event,
                                                        GetFocusManager());
}

bool NativeAppWindowViews::IsFrameless() const { return frameless_; }

bool NativeAppWindowViews::HasFrameColor() const { return false; }

SkColor NativeAppWindowViews::FrameColor() const { return SK_ColorBLACK; }

gfx::Insets NativeAppWindowViews::GetFrameInsets() const {
  if (frameless_)
    return gfx::Insets();

  // The pretend client_bounds passed in need to be large enough to ensure that
  // GetWindowBoundsForClientBounds() doesn't decide that it needs more than
  // the specified amount of space to fit the window controls in, and return a
  // number larger than the real frame insets. Most window controls are smaller
  // than 1000x1000px, so this should be big enough.
  gfx::Rect client_bounds = gfx::Rect(1000, 1000);
  gfx::Rect window_bounds =
      window_->non_client_view()->GetWindowBoundsForClientBounds(
          client_bounds);
  return window_bounds.InsetsFrom(client_bounds);
}

void NativeAppWindowViews::HideWithApp() {}

void NativeAppWindowViews::ShowWithApp() {}

void NativeAppWindowViews::UpdateShelfMenu() {}

gfx::Size NativeAppWindowViews::GetContentMinimumSize() const {
  return size_constraints_.GetMinimumSize();
}

gfx::Size NativeAppWindowViews::GetContentMaximumSize() const {
  return size_constraints_.GetMaximumSize();
}

void NativeAppWindowViews::SetContentSizeConstraints(
    const gfx::Size& min_size, const gfx::Size& max_size) {
  size_constraints_.set_minimum_size(min_size);
  size_constraints_.set_maximum_size(max_size);
}

}  // namespace apps