/*
* Copyright (c) 2015, Markus Achtelik, ASL, ETH Zurich, Switzerland
* You can contact the author at <markus dot achtelik at mavt dot ethz dot ch>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <mav_msgs/conversions.h>
#include <eigen_conversions/eigen_msg.h>

#include "mav_trajectory_generation_ros/ros_visualization.h"

namespace mav_trajectory_generation {

// Declare/define some internal functions that we don't want to expose.
namespace internal {

static constexpr double kDefaultDistance = 1.0;
static constexpr double kDefaultSamplingTime = 0.1;

void appendMarkers(const MarkerArray& markers_to_insert,
                   const std::string& marker_namespace,
                   MarkerArray* marker_array) {
  marker_array->markers.reserve(marker_array->markers.size() +
                                markers_to_insert.markers.size());
  for (const auto marker : markers_to_insert.markers) {
    marker_array->markers.push_back(marker);
    if (!marker_namespace.empty()) {
      marker_array->markers.back().ns = marker_namespace;
    }
  }
}

}  // end namespace internal

void drawMavTrajectory(const Trajectory& trajectory, double distance,
                       const std::string& frame_id,
                       visualization_msgs::MarkerArray* marker_array) {
  // This is just an empty extra marker that doesn't draw anything.
  mav_viz::MarkerGroup dummy_marker;
  return drawMavTrajectory(trajectory, distance, frame_id, dummy_marker,
                           marker_array);
}

void drawMavTrajectoryWithMavMarker(
    const Trajectory& trajectory, double distance, const std::string& frame_id,
    const mav_viz::MarkerGroup& additional_marker,
    visualization_msgs::MarkerArray* marker_array) {
  CHECK_NOTNULL(marker_array);
  marker_array->markers.clear();

  // Sample the trajectory.

  Marker line_strip;
  line_strip.type = Marker::LINE_STRIP;
  line_strip.color = mav_visualization::createColorRGBA(1, 0.5, 0, 1);
  line_strip.scale.x = 0.01;
  line_strip.ns = "path";

  double accumulated_distance = 0;
  Eigen::Vector3d last_position = Eigen::Vector3d::Zero();
  for (size_t i = 0; i < n_samples; ++i) {
    const mav_msgs::EigenTrajectoryPoint& flat_state = flat_states[i];

    accumulated_distance += (last_position - flat_state.position_W).norm();
    if (accumulated_distance > distance) {
      accumulated_distance = 0.0;
      mav_msgs::EigenMavState mav_state;
      mav_msgs::EigenMavStateFromEigenTrajectoryPoint(flat_state, &mav_state);

      MarkerArray axes_arrows;
      mav_viz::drawAxesArrows(axes_arrows, mav_state.position_W,
                              mav_state.orientation_W_B, 0.3, 0.3);
      internal::appendMarkers(axes_arrows, "pose", marker_array);

      Marker arrow;
      mav_viz::drawArrow(
          arrow, flat_state.position_W,
          flat_state.position_W + flat_state.acceleration_W,
          mav_visualization::createColorRGBA((190.0 / 255.0), (81.0 / 255.0),
                                   (80.0 / 255.0), 1),
          0.3);
      arrow.ns = positionDerivativeToString(ACCELERATION);
      marker_array->markers.push_back(arrow);

      mav_viz::drawArrow(
          arrow, flat_state.position_W,
          flat_state.position_W + flat_state.velocity_W,
          mav_visualization::createColorRGBA((80.0 / 255.0), (172.0 / 255.0),
                                   (196.0 / 255.0), 1),
          0.3);
      arrow.ns = positionDerivativeToString(VELOCITY);
      marker_array->markers.push_back(arrow);

      mav_viz::MarkerGroup tmp_marker(additional_marker);
      tmp_marker.transform(mav_state.position_W, mav_state.orientation_W_B);
      tmp_marker.getMarkers(marker_array->markers, 1.0, true);
    }
    last_position = flat_state.position_W;
    line_strip.points.push_back(mav_viz::eigenToPoint(last_position));
  }
  marker_array->markers.push_back(line_strip);

  std_msgs::Header header;
  header.frame_id = "world";
  header.stamp = ros::Time::now();
  setMarkerProperties(header, 0.0, Marker::ADD, marker_array);
}

bool drawVertices(const Vertex::Vector& vertices, const std::string& frame_id,
                  visualization_msgs::MarkerArray* marker_array) {
  CHECK_NOTNULL(marker_array);
  marker_array->markers.resize(1);
  Marker& marker = marker_array->markers.front();

  marker.type = Marker::LINE_STRIP;
  marker.color = mav_visualization::createColorRGBA(0.5, 1.0, 0, 1);
  marker.scale.x = 0.01;
  marker.ns = "straight_path";

  for (const Vertex& vertex : vertices) {
    if (vertex.D() != 3) {
      ROS_ERROR("Vertex has dimension %lu but should have dimension 3.",
                vertex.getDimension());
      return false;
    }

    if (vertex.hasConstraint(derivative_order::POSITION)) {
      Eigen::Vector3d position = Eigen::Vector3d::Zero();
      vertex.getConstraint(derivative_order::POSITION, &position);
      marker.points.push_back(mav_viz::eigenToPoint(position));
    } else
      ROS_WARN("Vertex does not have a position constraint, skipping.");
  }

  std_msgs::Header header;
  header.frame_id = frame_id;
  header.stamp = ros::Time::now();
  setMarkerProperties(header, 0.0, Marker::ADD, marker_array);
  return true;
}

}  // namespace mav_trajectory_generation