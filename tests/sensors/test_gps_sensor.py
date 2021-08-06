import holodeck
import uuid
from copy import deepcopy
import pytest
import numpy as np

from tests.utils.equality import almost_equal

@pytest.fixture
def config():
    c = {
        "name": "test_location_sensor",
        "world": "ExampleLevel",
        "main_agent": "sphere",
        "agents": [
            {
                "agent_name": "sphere",
                "agent_type": "SphereAgent",
                "sensors": [
                    {
                        "sensor_type": "GPSSensor",
                        "configuration": {}
                    }
                ],
                "control_scheme": 0,
                "location": [0,0,0]
            }
        ]
    }
    return c

@pytest.mark.parametrize('num', range(3))
def test_setting_depth(config, num):
    """Make sure if it's above the depth we receive data, and if below we don't
    """
    depth = np.random.rand()*10
    config["agents"][0]["sensors"][0]["configuration"]["Depth"] = depth

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    # Test above
    config["agents"][0]["location"] = [0, 0, -1*depth+1]
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:

        state = env.tick()
        assert "GPSSensor" in state

    # Test below
    config["agents"][0]["location"] = [0, 0, -1*depth-1]
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:

        state = env.tick()
        assert "GPSSensor" not in state

@pytest.mark.parametrize('num', range(3))
def test_random_depth(config, num):
    """Make sure the depth is changing according to the noise we put in
    """
    num_ticks = 100
    depth = np.random.rand()*10
    config["agents"][0]["sensors"][0]["configuration"]["Depth"] = depth
    config["agents"][0]["sensors"][0]["configuration"]["DepthStd"] = 1

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    # Test above
    config["agents"][0]["location"] = [0, 0, -1*depth]
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:

        count = 0
        for _ in num_ticks:
            state = env.tick()
            if "GPSSensor" in state:
                count += 1

        assert 0 < count
        assert count < num_ticks