// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef MODULES_WORLD_OPENDRIVE_COMMONS_HPP_
#define MODULES_WORLD_OPENDRIVE_COMMONS_HPP_

#include <inttypes.h>
#include <string>
#include <vector>

#include <boost/geometry.hpp>
#include "modules/geometry/line.hpp"

namespace modules {
namespace world {
namespace opendrive {

using XodrLaneId = uint32_t;
using XodrLanePosition = int32_t;
using XodrRoadId = uint32_t;

struct XodrRoadLinkInfo {
  XodrRoadLinkInfo() : id_(1000000), type_("") {}
  XodrRoadLinkInfo(const XodrRoadId &id,
               const std::string &type) :
    id_(id),
    type_(type) {}
  XodrRoadId id_;
  std::string type_;
};

struct XodrRoadLink {
  XodrRoadLink() : predecessor_(), successor_() {}
  XodrRoadLink(const XodrRoadLinkInfo &predecessor,
           const XodrRoadLinkInfo &successor) :
    predecessor_(predecessor),
    successor_(successor) {}
  XodrRoadLinkInfo predecessor_;
  XodrRoadLinkInfo successor_;
  //! getter
  XodrRoadLinkInfo get_predecessor() const { return predecessor_; }
  XodrRoadLinkInfo get_successor() const { return successor_; }
  void set_predecessor(const XodrRoadLinkInfo &info) { predecessor_ = info; }
  void set_successor(const XodrRoadLinkInfo &info) { successor_ = info; }
};

inline std::string print(const XodrRoadLink &l) {
  std::stringstream ss;
  ss << "XodrRoadLink.predecessor: " << l.predecessor_.id_ << \
        "of type" << l.predecessor_.type_ << "; ";
  ss << "XodrRoadLink.successor: " << l.successor_.id_ << \
        "of type" << l.successor_.type_ << std::endl;
  return ss.str();
}

struct XodrLaneOffset {
  float a, b, c, d;
};

inline float polynom(float x, float a, float b, float c, float d) {
  return a + b * x + c * x * x + d * x * x * x;
}

// TODO(@all): use type XodrLaneId here
struct XodrLaneLink {
  XodrLanePosition from_position;
  XodrLanePosition to_position;
};

inline std::string print(const XodrLaneLink &l) {
  std::stringstream ss;
  ss << "XodrLaneLink.from_position: " << l.from_position << "; ";
  ss << "XodrLaneLink.to_position: " << l.to_position << std::endl;
  return ss.str();
}

using XodrLaneLinks = std::vector<XodrLaneLink>;

struct Connection {
  void add_lane_link(XodrLaneLink link) { lane_links_.push_back(link); }
  XodrLaneLinks get_lane_links() const { return lane_links_; }
  uint32_t id_;
  uint32_t incoming_road_;  // TODO(@all): use type XodrRoadId here
  uint32_t connecting_road_;
  XodrLaneLinks lane_links_;
};

enum XodrDrivingDirection {
  FORWARD = 0,
  BACKWARD = 1,
  BOTH = 2
};

enum XodrLaneType {
  NONE = 0,
  DRIVING = 1,
  //STOP = 2,
  //SHOULDER = 3,
  BIKING = 4,
  SIDEWALK = 5,
  BORDER = 6,
  /*RESTRICTED = 7,
  PARKING = 8,
  BIDIRECTIONAL = 9,
  MEDIAN = 10,
  SPECIAL1 = 11,
  SPECIAL2 = 12,
  SPECIAL3 = 13,
  ROAD_WORKS = 14,
  TRAM = 15,
  RAIL = 16,
  ENTRY = 17,
  EXIT = 18,
  OFF_RAMP = 19,
  ON_RAMP = 20,
  CONNECTING_RAMP = 21,
  BUS = 22,
  TAXI = 23,
  HOV = 24
  */
};

namespace roadmark {

enum XodrRoadMarkType {
  NONE = 0,
  SOLID = 1,
  BROKEN = 2,
  /*SOLID_SOLID = 3, // (for double solid line)
  SOLID_BROKEN = 4, // (from inside to outside, exception: center lane – from left to right)
  BROKEN_SOLID = 5, // (from inside to outside, exception: center lane – from left to right)
  BROKEN_BROKEN = 6, // (from inside to outside, exception: center lane – from left to right)
  BOTTS_DOTS = 7,
  GRASS = 8, // (meaning a grass edge)
  CURB = 9,
  CUSTOM = 10, //  (if detailed description is given in child tags)
  EDGE = 11, // (describing the limit of usable space on a road)
  */
};

enum XodrRoadMarkColor {
  STANDARD = 0, // (equivalent to "white")
  /*BLUE = 1,
  GREEN = 2,
  RED = 3,
  */
  WHITE = 4,
  YELLOW = 5,
  //ORANGE = 6,
};

} // namespace roadmark

struct XodrRoadMark {
  roadmark::XodrRoadMarkType type_;
  roadmark::XodrRoadMarkColor color_;
  float width_;
};

inline std::string print(const XodrRoadMark &r) {
  std::stringstream ss;
  ss << "XodrRoadMark: type: " << r.type_ << ", color: " << \
        r.color_ << ", width: " << r.width_ << std::endl;
  return ss.str();
}

struct XodrLaneWidth {
  float s_start;
  float s_end;
  XodrLaneOffset off;
};

inline geometry::Line create_line_with_offset_from_line(
  geometry::Line previous_line,
  int id,
  XodrLaneWidth lane_width_current_lane,
  float s_inc = 0.5f) {

  namespace bg = boost::geometry;
  XodrLaneOffset off = lane_width_current_lane.off;
  float s = lane_width_current_lane.s_start;
  float s_end = lane_width_current_lane.s_end;
  float scale = 0.0f;

  boost::geometry::unique(previous_line.obj_);
  previous_line.recompute_s();

  geometry::Line tmp_line;
  geometry::Point2d normal(0.0f, 0.0f);
  int sign = id > 0 ? -1 : 1;
  if (s_end > previous_line.length())
    s_end = previous_line.length();

  for (; s <= s_end;) {
    geometry::Point2d point = get_point_at_s(previous_line, s);
    normal = get_normal_at_s(previous_line, s);
    scale = -sign * polynom(
      s-lane_width_current_lane.s_start, off.a, off.b, off.c, off.d);
    tmp_line.add_point(
      geometry::Point2d(bg::get<0>(point) + scale * bg::get<0>(normal),
                        bg::get<1>(point) + scale * bg::get<1>(normal)));
    if ((s_end - s < s_inc) &&
        (s_end - s > 0.))
      s_inc = s_end - s;
    s += s_inc;
  }

  return tmp_line;
}

//using XodrLaneWidths = std::vector<XodrLaneWidth>;

}  // namespace opendrive
}  // namespace world
} // namespace modules

#endif // MODULES_WORLD_OPENDRIVE_COMMONS_HPP_
