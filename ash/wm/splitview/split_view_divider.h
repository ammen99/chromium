// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLITSVIEW_SPLIT_VIEW_DIVIDER_H_
#define ASH_WM_SPLITSVIEW_SPLIT_VIEW_DIVIDER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/wm/public/activation_change_observer.h"

namespace views {
class Widget;
}  // namespace views

namespace aura {
class ScopedWindowTargeter;
}  // namespace aura

namespace ash {

class SplitViewController;

// Split view divider. It passes the mouse/gesture events to SplitViewController
// to resize the left and right windows accordingly. The divider widget should
// always placed above its observed windows to be able to receive events.
class ASH_EXPORT SplitViewDivider : public aura::WindowObserver,
                                    public ::wm::ActivationChangeObserver,
                                    public display::DisplayObserver {
 public:
  SplitViewDivider(SplitViewController* controller, aura::Window* root_window);
  ~SplitViewDivider() override;

  // Gets the size of the divider widget. The divider widget is enlarged during
  // dragging. For now, it's a vertical rectangle.
  static gfx::Size GetDividerSize(const gfx::Rect& work_area_bounds,
                                  bool is_dragging);

  // Updates |divider_widget_|'s bounds.
  void UpdateDividerBounds(bool is_dragging);

  // Calculates the divider's bounds according to the divider's position.
  gfx::Rect GetDividerBoundsInScreen(bool is_dragging);

  void AddObservedWindow(aura::Window* window);
  void RemoveObservedWindow(aura::Window* window);

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // wm::ActivationChangeObserver:
  void OnWindowActivated(ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  views::Widget* divider_widget() { return divider_widget_.get(); }

 private:
  void CreateDividerWidget(aura::Window* root_window);

  SplitViewController* controller_;

  // The window targeter that is installed on the always on top container window
  // when the split view mode is active. It deletes itself when the split view
  // mode is ended. Upon destruction, it restores the previous window targeter
  // (if any) on the always on top container window.
  std::unique_ptr<aura::ScopedWindowTargeter> split_view_window_targeter_;

  // Split view divider widget. It's a black bar stretching from one edge of the
  // screen to the other, containing a small white drag bar in the middle. As
  // the user presses on it and drag it to left or right, the left and right
  // window will be resized accordingly.
  std::unique_ptr<views::Widget> divider_widget_;

  // Tracks observed windows.
  aura::Window::Windows observed_windows_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewDivider);
};

}  // namespace ash

#endif  // ASH_WM_SPLITSVIEW_SPLIT_VIEW_DIVIDER_H_
