from typing import Callable, List

import holoocean
import pytest
from holoocean import packagemanager as pm
from holoocean.environments import HolodeckEnvironment


def pytest_generate_tests(metafunc):
    """Iterate over every scenario
    """
    scenarios = set()
    for config, full_path in pm._iter_packages():
        for world_entry in config["worlds"]:
            for config, full_path in pm._iter_scenarios(world_entry["name"]):
                scenarios.add("{}-{}".format(config["world"], config["name"]))

    if "scenario" in metafunc.fixturenames:
        metafunc.parametrize("scenario", scenarios)
    elif "env_scenario" in metafunc.fixturenames:
        metafunc.parametrize("env_scenario", scenarios, indirect=True)


# Envs contains a mapping of scenario key -> HolodeckEnvironment so that
# between different tests the same environment doesn't have to be created
# over and over
envs = {}


@pytest.fixture
def env_scenario(request):
    """Gets an environment for the scenario matching request.param. Creates the
    env or uses a cached one. Calls .reset() for you.
    """
    global envs
    scenario = request.param
    if scenario in envs:
        env = envs[scenario]
        env.reset()
        return env, scenario

    env = holoocean.make(scenario, show_viewport=False, frames_per_sec=False)
    env.reset()
    envs[scenario] = env
    return env, scenario
