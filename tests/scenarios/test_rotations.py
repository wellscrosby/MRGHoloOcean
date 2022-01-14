import holoocean
import uuid
import pytest
import numpy as np
from scipy.spatial.transform import Rotation

@pytest.fixture(scope="module")
def env():
    scenario = {
        "name": "test_location_sensor",
        "world": "ExampleLevel",
        "main_agent": "sphere",
        "agents": [
            {
                "agent_name": "sphere",
                "agent_type": "SphereAgent",
                "sensors": [
                    {
                        "sensor_type": "OrientationSensor",
                        "rotation": [0,0,0]
                    }
                ],
                "control_scheme": 0,
                "location": [0,0,0],
                "rotation": [0,0,0]
            }
        ]
    }
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    with holoocean.environments.HoloOceanEnvironment(scenario=scenario,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        yield env


@pytest.mark.parametrize('num', range(3))
def test_actor_rotation(env, num):
    """Make sure the orientation we tell the actor to start at is the correct one
    """
    angles = np.random.rand(3)*180
    R = Rotation.from_euler('xyz', angles, degrees=True).as_matrix()

    env._scenario["agents"][0]["rotation"] = angles.tolist()
    env._scenario["agents"][0]["sensors"][0]["rotation"] = [0, 0, 0]

    env.reset()

    state = env.tick()

    assert np.allclose(R, state["OrientationSensor"], atol=1e-4)


@pytest.mark.parametrize('num', range(3))
def test_sensor_rotation(env, num):
    """Make sure the orientation we tell the sensor to start at is the correct one
    """
    angles = np.random.rand(3)*180
    
    R = Rotation.from_euler('xyz', angles, degrees=True).as_matrix()

    env._scenario["agents"][0]["rotation"] = [0, 0, 0]
    env._scenario["agents"][0]["sensors"][0]["rotation"] = angles.tolist()

    env.reset()

    state = env.tick()

    assert np.allclose(R, state["OrientationSensor"], atol=1e-4)


@pytest.mark.parametrize('num', range(3))
def test_teleport_rotation(env, num):
    """Make sure the orientation we teleport the agent to is the correct one
    """
    angles = np.random.rand(3)*180
    
    R = Rotation.from_euler('xyz', angles, degrees=True).as_matrix()

    env._scenario["agents"][0]["rotation"] = [0, 0, 0]
    env._scenario["agents"][0]["sensors"][0]["rotation"] = [0,0,0]

    env.reset()

    state = env.tick()
    env.agents['sphere'].teleport([0,0,0], angles.tolist())
    state = env.tick()

    assert np.allclose(R, state["OrientationSensor"], atol=1e-4)