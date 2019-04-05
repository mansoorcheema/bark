// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include <utility>
#include "modules/world/map/local_map.hpp"
#include "modules/world/map/map_interface.hpp"

namespace modules {
namespace world {
namespace map {

using modules::world::opendrive::LanePtr;
using geometry::get_nearest_idx;
using geometry::distance;

void LocalMap::concatenate_lines(const std::vector<LanePtr>& lanes,
                                 Line& line_of_corridor) {
  if (lanes.size() > 0) {
      line_of_corridor = lanes.at(0)->get_line();
      for (int i = 1; i < lanes.size(); i++) {
        if (lanes.at(i) != NULL) {
          line_of_corridor.concatenate_linestring(lanes.at(i)->get_line());
        }
      }
  }
}

bool LocalMap::generate(Point2d point,
                        LaneId goal_lane_id,
                        double horizon) {
  std::vector<LanePtr> lanes;
  map_interface_->get_nearest_lanes(point, 1, lanes);
  LanePtr current_lane = lanes.at(0);

  std::pair< std::vector<LanePtr>, std::vector<LanePtr> > route =
    map_interface_->get_lane_boundary_horizon(current_lane->get_id(),
                                              goal_lane_id_);

  concatenate_lines(route.first, driving_corridor_.inner);
  concatenate_lines(route.second, driving_corridor_.outer);

  if (route.first[0] != NULL && route.second[0] != NULL) {
    driving_corridor_.center =
      geometry::calculate_center_line(driving_corridor_.inner,
                                      driving_corridor_.outer);
  }
  driving_corridor_.computed = true;
}

Line LocalMap::line_horizon(const Line& line,
                            const Point2d& p,
                            double horizon) {
  // TODO(@hart): do not access via member obj_
  Line new_line;
  int nearest_idx = get_nearest_idx(line, p);
  int max_idx = line.obj_.size();
  double s = 0.0;
  for (int idx = nearest_idx; nearest_idx < max_idx - 1; idx++) {
    double d = distance(line.obj_.at(idx), line.obj_.at(idx+1));
    new_line.add_point(line.obj_.at(idx+1));
    s += d;
    if ( s > horizon )
      return new_line;
  }
  return new_line;
}

bool LocalMap::compute_horizon_corridor(const Point2d& p, double horizon) {
  // TODO(@hart): create horizon lines for the current driving corridor
  horizon_driving_corridor_.inner = line_horizon(driving_corridor_.inner,
                                                 p,
                                                 horizon);
  horizon_driving_corridor_.outer = line_horizon(driving_corridor_.outer,
                                                 p,
                                                 horizon);
  if (horizon_driving_corridor_.inner.obj_.size() > 0 &&
      horizon_driving_corridor_.outer.obj_.size() > 0) {
    horizon_driving_corridor_.center =
      geometry::calculate_center_line(horizon_driving_corridor_.inner,
                                      horizon_driving_corridor_.outer);
    return true;
  }
  return false;
}

}  // namespace map
}  // namespace world
}  // namespace modules
