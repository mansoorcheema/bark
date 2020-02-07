// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick
// Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "modules/world/observed_world.hpp"
#include "gtest/gtest.h"
#include "modules/commons/params/default_params.hpp"
#include "modules/commons/params/setter_params.hpp"
#include "modules/geometry/polygon.hpp"
#include "modules/geometry/standard_shapes.hpp"
#include "modules/models/behavior/constant_velocity/constant_velocity.hpp"
#include "modules/models/behavior/motion_primitives/motion_primitives.hpp"
#include "modules/models/dynamic/single_track.hpp"
#include "modules/models/execution/interpolation/interpolate.hpp"
#include "modules/world/evaluation/evaluator_collision_agents.hpp"
#include "modules/world/goal_definition/goal_definition.hpp"
#include "modules/world/goal_definition/goal_definition_polygon.hpp"
#include "modules/world/map/map_interface.hpp"
#include "modules/world/map/roadgraph.hpp"
#include "modules/world/objects/agent.hpp"
#include "modules/world/opendrive/opendrive.hpp"
#include "modules/world/tests/dummy_road_corridor.hpp"
#include "modules/world/tests/make_test_world.hpp"
#include "modules/world/tests/make_test_xodr_map.hpp"

using namespace modules::models::dynamic;
using namespace modules::models::behavior;
using namespace modules::models::execution;
using namespace modules::world::map;

using modules::commons::DefaultParams;
using modules::commons::SetterParams;
using modules::commons::transformation::FrenetPosition;
using modules::geometry::Model3D;
using modules::geometry::Point2d;
using modules::geometry::Polygon;
using modules::geometry::Pose;
using modules::geometry::standard_shapes::CarRectangle;
using modules::world::FrontRearAgents;
using modules::world::ObservedWorld;
using modules::world::ObservedWorldPtr;
using modules::world::World;
using modules::world::WorldPtr;
using modules::world::goal_definition::GoalDefinitionPolygon;
using modules::world::objects::Agent;
using modules::world::objects::AgentPtr;
using modules::world::opendrive::OpenDriveMapPtr;
using modules::world::tests::MakeXodrMapOneRoadTwoLanes;
using StateDefinition::MIN_STATE_SIZE;

TEST(observed_world, agent_in_front_same_lane) {
  auto params = std::make_shared<DefaultParams>();

  // Setting Up Map
  OpenDriveMapPtr open_drive_map = MakeXodrMapOneRoadTwoLanes();
  MapInterfacePtr map_interface = std::make_shared<MapInterface>();
  map_interface->interface_from_opendrive(open_drive_map);

  // Goal Definition
  Polygon polygon(
      Pose(1, 1, 0),
      std::vector<Point2d>{Point2d(0, 0), Point2d(0, 2), Point2d(2, 2),
                           Point2d(2, 0), Point2d(0, 0)});
  std::shared_ptr<Polygon> goal_polygon(
      std::dynamic_pointer_cast<Polygon>(polygon.Translate(Point2d(50, -2))));
  auto goal_ptr = std::make_shared<GoalDefinitionPolygon>(*goal_polygon);

  // Setting Up Agents (one in front of another)
  ExecutionModelPtr exec_model(new ExecutionModelInterpolate(params));
  DynamicModelPtr dyn_model(new SingleTrackModel(params));
  BehaviorModelPtr beh_model(new BehaviorConstantVelocity(params));
  Polygon car_polygon = CarRectangle();

  State init_state1(static_cast<int>(MIN_STATE_SIZE));
  init_state1 << 0.0, 3.0, -1.75, 0.0, 5.0;
  AgentPtr agent1(new Agent(init_state1, beh_model, dyn_model, exec_model,
                            car_polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  State init_state2(static_cast<int>(MIN_STATE_SIZE));
  init_state2 << 0.0, 10.0, -1.75, 0.0, 5.0;
  AgentPtr agent2(new Agent(init_state2, beh_model, dyn_model, exec_model,
                            car_polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  // Construct World
  WorldPtr world(new World(params));
  world->AddAgent(agent1);
  world->AddAgent(agent2);
  world->UpdateAgentRTree();

  WorldPtr current_world_state1(world->Clone());
  ObservedWorld obs_world1(current_world_state1, agent2->GetAgentId());

  // Leading agent should not have an agent in front
  std::pair<AgentPtr, FrenetPosition> leading_vehicle = obs_world1.GetAgentInFront();
  EXPECT_FALSE(static_cast<bool>(leading_vehicle.first));

  // Leading agent should not have an agent in front
  std::pair<AgentPtr, FrenetPosition> following_vehicle = obs_world1.GetAgentBehind();
  BARK_EXPECT_TRUE(static_cast<bool>(following_vehicle.first));
  EXPECT_EQ(following_vehicle.first->GetAgentId(), agent1->GetAgentId());

  WorldPtr current_world_state2(world->Clone());
  ObservedWorld obs_world2(current_world_state2, agent1->GetAgentId());

  // Agent behind should have leading agent in front
  std::pair<AgentPtr, FrenetPosition> leading_vehicle2 = obs_world2.GetAgentInFront();
  EXPECT_TRUE(static_cast<bool>(leading_vehicle2.first));
  EXPECT_EQ(leading_vehicle2.first->GetAgentId(), agent2->GetAgentId());

  State init_state3(static_cast<int>(MIN_STATE_SIZE));
  init_state3 << 0.0, 20.0, -1.75, 0.0, 5.0;
  AgentPtr agent3(new Agent(init_state3, beh_model, dyn_model, exec_model,
                            polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT
  world->AddAgent(agent3);
  world->UpdateAgentRTree();

  WorldPtr current_world_state3(world->Clone());
  ObservedWorld obs_world3(current_world_state3, agent1->GetAgentId());

  // Adding a third agent in front of leading agent, still leading agent
  // should be in front
  std::pair<AgentPtr, FrenetPosition> leading_vehicle3 = obs_world3.GetAgentInFront();
  EXPECT_TRUE(static_cast<bool>(leading_vehicle3.first));
  EXPECT_EQ(leading_vehicle2.first->GetAgentId(), agent2->GetAgentId());
}

TEST(observed_world, agent_in_front_other_lane) {
  auto params = std::make_shared<DefaultParams>();

  // Setting Up Map
  OpenDriveMapPtr open_drive_map = MakeXodrMapOneRoadTwoLanes();
  MapInterfacePtr map_interface = std::make_shared<MapInterface>();
  map_interface->interface_from_opendrive(open_drive_map);

  // Goal Definition
  Polygon polygon(
      Pose(1, 1, 0),
      std::vector<Point2d>{Point2d(0, 0), Point2d(0, 2), Point2d(2, 2),
                           Point2d(2, 0), Point2d(0, 0)});
  std::shared_ptr<Polygon> goal_polygon(
      std::dynamic_pointer_cast<Polygon>(polygon.Translate(Point2d(50, -2))));
  auto goal_ptr = std::make_shared<GoalDefinitionPolygon>(*goal_polygon);

  // Setting Up Agents (one in front of another)
  ExecutionModelPtr exec_model(new ExecutionModelInterpolate(params));
  DynamicModelPtr dyn_model(new SingleTrackModel(params));
  BehaviorModelPtr beh_model(new BehaviorConstantVelocity(params));
  Polygon car_polygon = CarRectangle();

  State init_state1(static_cast<int>(MIN_STATE_SIZE));
  init_state1 << 0.0, 3.0, -1.75, 0.0, 5.0;
  AgentPtr agent1(new Agent(init_state1, beh_model, dyn_model, exec_model,
                            car_polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  State init_state2(static_cast<int>(MIN_STATE_SIZE));
  init_state2 << 0.0, 10.0, -1.75, 0.0, 5.0;
  AgentPtr agent2(new Agent(init_state2, beh_model, dyn_model, exec_model,
                            car_polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  // Construct World
  WorldPtr world(new World(params));
  world->AddAgent(agent1);
  world->AddAgent(agent2);
  world->UpdateAgentRTree();

  // Adding a fourth agent in right lane
  State init_state4(static_cast<int>(MIN_STATE_SIZE));
  init_state4 << 0.0, 5.0, -5.25, 0.0, 5.0;
  AgentPtr agent4(new Agent(init_state4, beh_model, dyn_model, exec_model,
                            polygon, params, goal_ptr, map_interface,
                            Model3D()));  // NOLINT

  world->AddAgent(agent4);
  world->UpdateAgentRTree();

  WorldPtr current_world_state4(world->Clone());
  ObservedWorld obs_world4(current_world_state4, agent4->GetAgentId());

  // there is no agent in front of agent4
  std::pair<AgentPtr, FrenetPosition> leading_vehicle4 = obs_world4.GetAgentInFront();
  EXPECT_FALSE(static_cast<bool>(leading_vehicle4.first));

  const auto& road_corridor4 = agent4->GetRoadCorridor();
  BARK_EXPECT_TRUE(road_corridor4 != nullptr);

  Point2d ego_pos4 = agent4->GetCurrentPosition();
  const auto& left_right_lane_corridor =
      road_corridor4->GetLeftRightLaneCorridor(ego_pos4);
  const LaneCorridorPtr& lane_corridor4 = left_right_lane_corridor.first;
  BARK_EXPECT_TRUE(lane_corridor4 != nullptr);

  // in the lane corridor left of agent4, there is agent2 in front
  FrontRearAgents fr_vehicle4b =
      obs_world4.GetAgentFrontRearForId(agent4->GetAgentId(), lane_corridor4);
  EXPECT_TRUE(static_cast<bool>(fr_vehicle4b.front.first));
  EXPECT_EQ(fr_vehicle4b.front.first->GetAgentId(), agent2->GetAgentId());

  // in the lane corridor left of agent4, there is agent1 behind
  EXPECT_TRUE(static_cast<bool>(fr_vehicle4b.rear.first));
  EXPECT_EQ(fr_vehicle4b.rear.first->GetAgentId(), agent1->GetAgentId());
}

TEST(observed_world, clone) {
  using modules::world::evaluation::EvaluatorCollisionAgents;
  using modules::world::evaluation::EvaluatorPtr;

  auto params = std::make_shared<DefaultParams>();
  ExecutionModelPtr exec_model(new ExecutionModelInterpolate(params));
  DynamicModelPtr dyn_model(new SingleTrackModel(params));
  BehaviorModelPtr beh_model(new BehaviorConstantVelocity(params));
  EvaluatorPtr col_checker(new EvaluatorCollisionAgents());

  Polygon polygon(
      Pose(1.25, 1, 0),
      std::vector<Point2d>{Point2d(0, 0), Point2d(0, 2), Point2d(4, 2),
                           Point2d(4, 0), Point2d(0, 0)});

  State init_state1(static_cast<int>(MIN_STATE_SIZE));
  init_state1 << 0.0, 0.0, 0.0, 0.0, 5.0;
  AgentPtr agent1(new Agent(init_state1, beh_model, dyn_model, exec_model,
                            polygon, params));  // NOLINT

  State init_state2(static_cast<int>(MIN_STATE_SIZE));
  init_state2 << 0.0, 8.0, 0.0, 0.0, 5.0;
  AgentPtr agent2(new Agent(init_state2, beh_model, dyn_model, exec_model,
                            polygon, params));  // NOLINT

  WorldPtr world = std::make_shared<World>(params);
  world->AddAgent(agent1);
  world->AddAgent(agent2);
  world->UpdateAgentRTree();

  WorldPtr current_world_state(world->Clone());
  ObservedWorldPtr observed_world(
      new ObservedWorld(current_world_state, agent1->GetAgentId()));

  WorldPtr cloned(observed_world->Clone());
  ObservedWorldPtr cloned_observed_world =
      std::dynamic_pointer_cast<ObservedWorld>(cloned);
  EXPECT_EQ(observed_world->GetEgoAgent()->GetAgentId(),
            cloned_observed_world->GetEgoAgent()->GetAgentId());
  EXPECT_EQ(typeid(observed_world->GetEgoBehaviorModel()),
            typeid(cloned_observed_world->GetEgoBehaviorModel()));

  observed_world.reset();
  auto behavior_ego = cloned_observed_world->GetEgoBehaviorModel();
  EXPECT_TRUE(behavior_ego != nullptr);
}

TEST(observed_world, predict) {
  using modules::models::behavior::BehaviorMotionPrimitives;
  using modules::models::behavior::DiscreteAction;
  using modules::models::dynamic::Input;
  using modules::world::prediction::PredictionSettings;
  using modules::world::tests::make_test_observed_world;
  using StateDefinition::VEL_POSITION;

  auto params = std::make_shared<SetterParams>();
  params->SetReal("integration_time_delta", 0.01);
  DynamicModelPtr dyn_model(new SingleTrackModel(params));
  float ego_velocity = 5.0, rel_distance = 7.0, velocity_difference = 0.0;
  auto observed_world = make_test_observed_world(1, rel_distance, ego_velocity,
                                                 velocity_difference);

  // predict all agents with constant velocity
  BehaviorModelPtr prediction_model(new BehaviorConstantVelocity(params));
  PredictionSettings prediction_settings(prediction_model, prediction_model);
  observed_world.SetupPrediction(prediction_settings);
  WorldPtr predicted_world = observed_world.Predict(1.0f);
  ObservedWorldPtr observed_predicted_world =
      std::dynamic_pointer_cast<ObservedWorld>(predicted_world);
  double distance_ego = modules::geometry::Distance(
      observed_predicted_world->CurrentEgoPosition(),
      observed_world.CurrentEgoPosition());
  double distance_other = modules::geometry::Distance(
      observed_predicted_world->GetOtherAgents()
          .begin()
          ->second->GetCurrentPosition(),  // NOLINT
      observed_world.GetOtherAgents().begin()->second->GetCurrentPosition());

  // distance current and predicted state should be
  // velocity x prediction time span
  EXPECT_NEAR(distance_ego, ego_velocity * 1.0f, 0.06);
  EXPECT_NEAR(distance_other,
              observed_world.GetOtherAgents()
                      .begin()
                      ->second->GetCurrentState()[VEL_POSITION] *
                  1.0f,  // NOLINT
              0.06);

  // predict ego agent with motion primitive model
  BehaviorModelPtr ego_prediction_model(
      new BehaviorMotionPrimitives(dyn_model, params));
  Input u1(2);
  u1 << 2, 0;
  Input u2(2);
  u2 << 0, 1;
  BehaviorMotionPrimitives::MotionIdx idx1 =
      std::dynamic_pointer_cast<BehaviorMotionPrimitives>(ego_prediction_model)
          ->AddMotionPrimitive(u1);  // NOLINT
  BehaviorMotionPrimitives::MotionIdx idx2 =
      std::dynamic_pointer_cast<BehaviorMotionPrimitives>(ego_prediction_model)
          ->AddMotionPrimitive(u2);  // NOLINT

  BehaviorModelPtr others_prediction_model(
      new BehaviorConstantVelocity(params));
  PredictionSettings prediction_settings2(ego_prediction_model,
                                          others_prediction_model);
  observed_world.SetupPrediction(prediction_settings2);
  WorldPtr predicted_world2 =
      observed_world.Predict(1.0f, DiscreteAction(idx1));
  auto ego_pred_velocity =
      std::dynamic_pointer_cast<ObservedWorld>(predicted_world2)
          ->CurrentEgoState()[StateDefinition::VEL_POSITION];  // NOLINT
  // distance current and predicted state should be velocity
  // + prediction time span
  EXPECT_NEAR(ego_pred_velocity, ego_velocity + 2 * 1.0f, 0.05);
}
