import holoocean
import uuid
import os
import pytest
import numpy as np

@pytest.fixture
def config():
    c = {
        "name": "test_location_sensor",
        "world": "ExampleLevel",
        "main_agent": "auv0",
        "frames_per_sec": False,
        "agents": [
            {
                "agent_name": "auv0",
                "agent_type": "HoveringAUV",
                "sensors": [
                    {
                        "sensor_type": "SonarSensor",
                        "configuration": {
                            "MinRange": .1,
                            "MaxRange": 1
                        }
                    }
                ],
                "control_scheme": 0,
                "location": [.95, -1.75, .5]
            }
        ]
    }
    return c

@pytest.mark.parametrize("size", [(.02, 5.12), (.1, 12.8), (.05, 25.6)])
def test_folder_creation(size, config):
    """Make sure folders are made with the correct size
    """

    config["octree_min"] = size[0]
    config["octree_max"] = size[1]

    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")

    with holoocean.environments.HoloOceanEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        for _ in range(10):
            env.tick()

    dir = os.path.join(holoocean.util.get_holodeck_path(), "worlds/Ocean/LinuxNoEditor/Holodeck/Octrees")
    dir = os.path.join(dir, f"{config['world']}/min{int(size[0]*100)}_max{int(size[1]*100)}")

    assert os.path.isdir(dir), "Sonar folder wasn't created"

def test_blank(config):
    """Test to make sure when we're in the middle of nowhere, we get nothing back"""

    config["agents"][0]["location"] = [-100, -100, -100]

    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")

    with holoocean.environments.HoloOceanEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        for _ in range(10):
            state = env.tick()["SonarSensor"]
            assert np.allclose(np.zeros_like(state), state)