# Copyright (c) 2019 fortiss GmbH
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT




from modules.runtime.scenario.scenario_generation.uniform_vehicle_distribution import UniformVehicleDistribution
from modules.runtime.viewer.pygame_viewer import PygameViewer
from modules.runtime.viewer.matplotlib_viewer import MPViewer
from modules.runtime.commons.parameters import ParameterServer
import time


param_server = ParameterServer()

viewer = PygameViewer(params=param_server, x_range=[-100, 100], y_range=[-100, 100])

sim_step_time = param_server["simulation"]["step_time",
                                        "Step-time used in simulation",
                                        1]
sim_real_time_factor = param_server["simulation"]["real_time_factor",
                                                "execution in real-time or faster",
                                                1]
scenario_generation = UniformVehicleDistribution(num_scenarios=3, random_seed=0, params=param_server)


for _ in range(0,5): # run 5 scenarios in a row, repeating after 3
    scenario = scenario_generation.get_next_scenario()
    for _ in range(0, 3): # run each scenario for 3 steps
        scenario.world_state.step(sim_step_time)
        viewer.drawWorld(scenario.world_state)
        viewer.show(block=False)
        time.sleep(sim_step_time/sim_real_time_factor)
