// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/views/widget/root_view.h"

#include "base/base_drag_source.h"
#include "base/logging.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/drag_drop_types.h"
#include "chrome/views/widget/root_view_drop_target.h"

namespace views {

RECT RootView::GetScheduledPaintRectConstrainedToSize() {
  if (invalid_rect_.IsEmpty())
    return invalid_rect_.ToRECT();

  return invalid_rect_.Intersect(GetLocalBounds(true)).ToRECT();
}

void RootView::UpdateCursor(const MouseEvent& e) {
  View *v = GetViewForPoint(e.location());

  if (v && v != this) {
    gfx::Point l(e.location());
    View::ConvertPointToView(this, v, &l);
    HCURSOR cursor = v->GetCursorForPoint(e.GetType(), l.x(), l.y());
    if (cursor) {
      ::SetCursor(cursor);
      return;
    }
  }
  if (previous_cursor_) {
    SetCursor(previous_cursor_);
  }
}

void RootView::OnPaint(HWND hwnd) {
  gfx::Rect original_dirty_region = GetScheduledPaintRectConstrainedToSize();
  if (!original_dirty_region.empty()) {
    // Invoke InvalidateRect so that the dirty region of the window includes the
    // region we need to paint. If we didn't do this and the region didn't
    // include the dirty region, ProcessPaint would incorrectly mark everything
    // as clean. This can happen if a WM_PAINT is generated by the system before
    // the InvokeLater schedule by RootView is processed.
    RECT win_version = original_dirty_region.ToRECT();
    InvalidateRect(hwnd, &win_version, FALSE);
  }
  ChromeCanvasPaint canvas(hwnd);
  if (!canvas.isEmpty()) {
    const PAINTSTRUCT& ps = canvas.paintStruct();
    SchedulePaint(gfx::Rect(ps.rcPaint), false);
    if (NeedsPainting(false))
      ProcessPaint(&canvas);
  }
}

bool RootView::GetAccessibleRole(VARIANT* role) {
  DCHECK(role);

  role->vt = VT_I4;
  role->lVal = ROLE_SYSTEM_APPLICATION;
  return true;
}

void RootView::StartDragForViewFromMouseEvent(
    View* view,
    IDataObject* data,
    int operation) {
  drag_view_ = view;
  scoped_refptr<BaseDragSource> drag_source(new BaseDragSource);
  DWORD effects;
  DoDragDrop(data, drag_source,
             DragDropTypes::DragOperationToDropEffect(operation), &effects);
  // If the view is removed during the drag operation, drag_view_ is set to
  // NULL.
  if (drag_view_ == view) {
    View* drag_view = drag_view_;
    drag_view_ = NULL;
    drag_view->OnDragDone();
  }
}

}
