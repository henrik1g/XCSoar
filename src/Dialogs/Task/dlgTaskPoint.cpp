/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Form/Draw.hpp"
#include "Form/Button.hpp"
#include "Widget/DockWindow.hpp"
#include "Widget/PanelWidget.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "Units/Units.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/StartPoint.hpp"
#include "Engine/Task/Ordered/Points/FinishPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/AnnularSectorZone.hpp"
#include "Task/TypeStrings.hpp"
#include "Gauge/TaskView.hpp"
#include "Compiler.h"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widgets/CylinderZoneEditWidget.hpp"
#include "Widgets/SectorZoneEditWidget.hpp"
#include "Widgets/LineSectorZoneEditWidget.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

#include <assert.h>
#include <stdio.h>

static WndForm *wf = NULL;
static WndFrame* wTaskView = NULL;
static DockWindow *dock;
static ObservationZoneEditWidget *properties_widget;
static OrderedTask* ordered_task = NULL;
static bool task_modified = false;
static unsigned active_index = 0;
static int next_previous = 0;

class TPOZListener : public ObservationZoneEditWidget::Listener {
public:
  /* virtual methods from class ObservationZoneEditWidget::Listener */
  virtual void OnModified(ObservationZoneEditWidget &widget) override;
};

static TPOZListener listener;

static ObservationZoneEditWidget *
CreateObservationZoneEditWidget(ObservationZonePoint &oz, bool is_fai_general)
{
  switch (oz.GetShape()) {
  case ObservationZonePoint::SECTOR:
  case ObservationZonePoint::ANNULAR_SECTOR:
    return new SectorZoneEditWidget((SectorZone &)oz);

  case ObservationZonePoint::LINE:
    return new LineSectorZoneEditWidget((LineSectorZone &)oz, !is_fai_general);

  case ObservationZonePoint::CYLINDER:
    return new CylinderZoneEditWidget((CylinderZone &)oz, !is_fai_general);

  case ObservationZonePoint::FAI_SECTOR:
  case ObservationZonePoint::KEYHOLE:
  case ObservationZonePoint::BGAFIXEDCOURSE:
  case ObservationZonePoint::BGAENHANCEDOPTION:
  case ObservationZonePoint::BGA_START:
    break;
  }

  return nullptr;
}

static void
RefreshView()
{
  wTaskView->Invalidate();

  OrderedTaskPoint &tp = ordered_task->GetPoint(active_index);

  dock->SetWidget(new PanelWidget());

  ObservationZonePoint &oz = tp.GetObservationZone();
  const bool is_fai_general =
    ordered_task->GetFactoryType() == TaskFactoryType::FAI_GENERAL;
  properties_widget = CreateObservationZoneEditWidget(oz, is_fai_general);
  if (properties_widget != nullptr) {
    properties_widget->SetListener(&listener);
    dock->SetWidget(properties_widget);
  }

  WndFrame* wfrm = NULL;
  wfrm = ((WndFrame*)wf->FindByName(_T("lblType")));
  if (wfrm)
    wfrm->SetCaption(OrderedTaskPointName(ordered_task->GetFactory().GetType(tp)));

  SetFormControlEnabled(*wf, _T("butPrevious"), active_index > 0);
  SetFormControlEnabled(*wf, _T("butNext"),
                        active_index < (ordered_task->TaskSize() - 1));

  WndButton* wb;
  wb = (WndButton*)wf->FindByName(_T("cmdOptionalStarts"));
  assert(wb);
  wb->SetVisible(active_index == 0);
  if (!ordered_task->HasOptionalStarts())
    wb->SetCaption(_("Enable Alternate Starts"));
  else {
    StaticString<50> tmp;
    tmp.Format(_T("%s (%d)"), _("Edit Alternates"),
               ordered_task->GetOptionalStartPointCount());
    wb->SetCaption(tmp);
  }

  StaticString<100> name_prefix_buffer, type_buffer;

  switch (tp.GetType()) {
  case TaskPointType::START:
    type_buffer = _T("Start point");
    name_prefix_buffer = _T("Start: ");
    break;

  case TaskPointType::AST:
    type_buffer = _T("Task point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPointType::AAT:
    type_buffer = _T("Assigned area point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPointType::FINISH:
    type_buffer = _T("Finish point");
    name_prefix_buffer = _T("Finish: ");
    break;

  default:
    gcc_unreachable();
  }

  wf->SetCaption(type_buffer);

  wfrm = ((WndFrame*)wf->FindByName(_T("lblLocation")));
  if (wfrm) {
    StaticString<100> buffer;
    buffer.Format(_T("%s %s"), name_prefix_buffer.c_str(),
                  tp.GetWaypoint().name.c_str());
    wfrm->SetCaption(buffer);
  }
}

static void
ReadValues()
{
  if (properties_widget != nullptr) {
    bool require_restart;
    properties_widget->Save(task_modified, require_restart);
  }
}

static void
OnTaskPaint(WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  PixelRect rc = Sender->GetClientRect();

  const OrderedTaskPoint &tp = ordered_task->GetPoint(active_index);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const MapLook &look = UIGlobals::GetMapLook();
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTaskPoint(canvas, rc, *ordered_task, tp,
                 basic.location_available, basic.location,
                 CommonInterface::GetMapSettings(),
                 look.task, look.airspace,
                 terrain, &airspace_database);
}

static void
OnRemoveClicked()
{
  if (ShowMessageBox(_("Remove task point?"), _("Task Point"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (!ordered_task->GetFactory().Remove(active_index))
    return;

  task_modified = true;
  wf->SetModalResult(mrCancel);
}

static void
OnDetailsClicked()
{
  const OrderedTaskPoint &task_point = ordered_task->GetPoint(active_index);
  dlgWaypointDetailsShowModal(wf->GetMainWindow(),
                              task_point.GetWaypoint(), false);
}

static void
OnRelocateClicked()
{
  const GeoPoint &gpBearing = active_index > 0
    ? ordered_task->GetPoint(active_index - 1).GetLocation()
    : CommonInterface::Basic().location;

  const Waypoint *wp = ShowWaypointListDialog(wf->GetMainWindow(), gpBearing,
                                         ordered_task, active_index);
  if (wp == NULL)
    return;

  ordered_task->GetFactory().Relocate(active_index, *wp);
  task_modified = true;
  RefreshView();
}

static void
OnTypeClicked()
{
  if (dlgTaskPointType(&ordered_task, active_index)) {
    task_modified = true;
    RefreshView();
  }
}

static void
OnPreviousClicked()
{
  if (active_index > 0) {
    next_previous=-1;
    wf->SetModalResult(mrOK);
  }
}

static void
OnNextClicked()
{
  if (active_index < (ordered_task->TaskSize() - 1)) {
    next_previous=1;
    wf->SetModalResult(mrOK);
  }
}

/**
 * displays dlgTaskOptionalStarts
 * @param Sender
 */
static void
OnOptionalStartsClicked()
{
  if (dlgTaskOptionalStarts(wf->GetMainWindow(), &ordered_task)) {
    task_modified = true;
    RefreshView();
  }
}

void
TPOZListener::OnModified(ObservationZoneEditWidget &widget)
{
  ReadValues();
  wTaskView->Invalidate();
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnRemoveClicked),
  DeclareCallBackEntry(OnRelocateClicked),
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnTypeClicked),
  DeclareCallBackEntry(OnPreviousClicked),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnOptionalStartsClicked),
  DeclareCallBackEntry(OnTaskPaint),
  DeclareCallBackEntry(NULL)
};

bool
dlgTaskPointShowModal(SingleWindow &parent, OrderedTask** task,
                      const unsigned index)
{
  ordered_task = *task;
  task_modified = false;
  active_index = index;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_TASKPOINT_L") :
                                      _T("IDR_XML_TASKPOINT"));
  assert(wf != NULL);

  wTaskView = (WndFrame*)wf->FindByName(_T("frmTaskView"));
  assert(wTaskView != NULL);

  dock = (DockWindow *)wf->FindByName(_T("properties"));
  assert(dock != nullptr);

  do {
    RefreshView();
    next_previous=0;
    if (wf->ShowModal() == mrOK)
      ReadValues();
    active_index += next_previous;
  } while (next_previous != 0);

  delete wf;

  if (*task != ordered_task) {
    *task = ordered_task;
    task_modified = true;
  } 
  if (task_modified) {
    ordered_task->UpdateGeometry();
  }
  return task_modified;
}
