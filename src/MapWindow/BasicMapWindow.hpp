/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#ifndef XCSOAR_BASIC_MAP_WINDOW_HPP
#define XCSOAR_BASIC_MAP_WINDOW_HPP

#include "Projection/MapWindowProjection.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Screen/BufferWindow.hpp"
#include "Screen/LabelBlock.hpp"
#include "MapWindowBlackboard.hpp"
#include "Renderer/BackgroundRenderer.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Compiler.h"

#ifndef ENABLE_OPENGL
#include "Screen/BufferCanvas.hpp"
#endif

struct WaypointLook;
struct TaskLook;
struct AircraftLook;
class ContainerWindow;
class TopographyStore;
class TopographyRenderer;
class Waypoints;
class Airspaces;
class ProtectedTaskManager;
class GlideComputer;

class BasicMapWindow : public BufferWindow {
#ifndef ENABLE_OPENGL
  // graphics vars

  BufferCanvas buffer_canvas;
  BufferCanvas stencil_canvas;
#endif

  MapWindowProjection projection;

  LabelBlock label_block;

  BackgroundRenderer background;
  TopographyRenderer *topography_renderer;

  AirspaceRenderer airspace_renderer;

  WaypointRenderer way_point_renderer;

public:
  BasicMapWindow(const WaypointLook &waypoint_look,
                 const AirspaceLook &airspace_look);
  virtual ~BasicMapWindow();

  void set(ContainerWindow &parent, PixelScalar left, PixelScalar top,
           UPixelScalar width, UPixelScalar height, WindowStyle style);

  void SetTerrain(RasterTerrain *terrain);
  void SetTopograpgy(TopographyStore *topography);

  void SetAirspaces(Airspaces *airspace_database) {
    airspace_renderer.SetAirspaces(airspace_database);
  }

  void SetWaypoints(const Waypoints *way_points) {
    way_point_renderer.set_way_points(way_points);
  }

  void SetTarget(GeoPoint location, fixed radius);

private:
  /**
   * Renders the terrain background
   * @param canvas The drawing canvas
   */
  void RenderTerrain(Canvas &canvas);

  /**
   * Renders the topography
   * @param canvas The drawing canvas
   */
  void RenderTopography(Canvas &canvas);

  /**
   * Renders the topography labels
   * @param canvas The drawing canvas
   */
  void RenderTopographyLabels(Canvas &canvas);

  /**
   * Renders the airspace
   * @param canvas The drawing canvas
   */
  void RenderAirspace(Canvas &canvas);

  void DrawWaypoints(Canvas &canvas);

protected:
  virtual void OnCreate();
  virtual void OnDestroy();
  virtual void OnResize(UPixelScalar width, UPixelScalar height);

  virtual void OnPaintBuffer(Canvas &canvas);
};

#endif
