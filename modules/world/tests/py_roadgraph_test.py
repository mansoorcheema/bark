# Copyright (c) 2019 fortiss GmbH
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT

import unittest
from bark.world.map import Roadgraph
from bark.world.opendrive import XodrLane
from modules.runtime.commons.xodr_parser import XodrParser


class RoadgraphTests(unittest.TestCase):
  def test_world(self):
    road_graph = Roadgraph()
    lane1 = XodrLane()
    lane2 = XodrLane()
    road_id = 0

    # add vertices and connect edges
    u = road_graph.AddLane(road_id, lane1)
    v = road_graph.AddLane(road_id, lane2)
    road_graph.AddInnerNeighbor(u, v)
    road_graph.AddOuterNeighbor(u, v)

    vertices = road_graph.GetVertices()
    for vertex_descr in vertices:
      vertex = road_graph.GetVertex(vertex_descr)
      print(vertex.lane_id)

    GetEdges = road_graph.GetEdges()
    for edge_descr in GetEdges:
      edge = road_graph.GetEdge(edge_descr)
      print(edge.edge_type)
  
 

if __name__ == '__main__':
    unittest.main()
