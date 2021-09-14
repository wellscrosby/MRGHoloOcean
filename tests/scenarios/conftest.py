from typing import Callable, List

import holoocean
import pytest
from holoocean import packagemanager as pm
from holoocean.environments import HoloOceanEnvironment


def pytest_generate_tests(metafunc):
    """Iterate over every scenario
    """
    scenarios = set()
    scenarios_single = set()
    worlds = set()
    for config, full_path in pm._iter_packages():
        for world_entry in config["worlds"]:
            for config, full_path in pm._iter_scenarios(world_entry["name"]):
                use = True
                name = "{}-{}".format(config["world"], config["name"])
                config = holoocean.packagemanager.get_scenario(name)

                # Take all without a sonar for loading test
                for agent in config['agents']:
                    for sensor in agent['sensors']:
                        if sensor["sensor_type"] == "SonarSensor":
                            use = False
                if use:
                    scenarios.add(name)

                # Take one from each world for the other tests
                if config['world'] not in worlds:
                    scenarios_single.add(name)
                    worlds.add(config['world'])

    if "scenario" in metafunc.fixturenames:
        metafunc.parametrize("scenario", scenarios)
    elif "env_scenario" in metafunc.fixturenames:
        metafunc.parametrize("env_scenario", scenarios_single, indirect=True)


@pytest.fixture
def env_scenario(request):
    """Gets an environment for the scenario matching request.param. Creates the
    env or uses a cached one. Calls .reset() for you.
    """
    scenario = request.param

    env = holoocean.make(scenario, show_viewport=False, frames_per_sec=False)
    env.reset()
    return env, scenario
