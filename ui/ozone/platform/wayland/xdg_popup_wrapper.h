// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_XDG_POPUP_WRAPPER_H_
#define UI_OZONE_PLATFORM_WAYLAND_XDG_POPUP_WRAPPER_H_

#include "ui/gfx/geometry/rect.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

namespace ui {

class WaylandConnection;

// A wrapper around different versions of xdg popups.
class XDGPopupWrapper {
 public:
  virtual ~XDGPopupWrapper() {}

  // Creates actual xdg popup object and sets a listener to it.
  virtual bool Initialize(WaylandConnection* connection,
                          wl_surface* surface,
                          wl_surface* parent_surface,
                          const gfx::Rect& bounds) = 0;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_XDG_POPUP_WRAPPER_H_
