import holodeck
import uuid
from holodeck.command import SendOpticalMessageCommand
from holodeck import sensors
import copy

from tests.utils.equality import almost_equal

uav_config_v1 = {
    "name": "test",
    "world": "TestWorld",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "LocationSensor",
                },
                {
                    "sensor_type": "VelocitySensor"
                },
                {
                    "sensor_type": "RGBCamera"
                },
                {
                    "sensor_type": "OpticalModemSensor"
                }
            ],
            "control_scheme": 1,
            "location": [0, 0, .5],
            "rotation": [0, 0, 0]
        },
        {
            "agent_name": "uav1",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor"
                }
            ],
            "control_scheme": 1,
            "location": [5, 0, 1],
            "rotation": [0, 0, -180]
        }
    ]
}

def test_transmittable():
    "Tests to make sure that two sensors that can transmit to each other actually transmit."
    
    binary_path = holodeck.packagemanager.get_binary_path_for_package("DefaultWorlds")


    with holodeck.environments.HolodeckEnvironment(scenario=uav_config_v1,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]

        state, reward, terminal, _ = env.step(command)
        env.agents.get("uav0")._client.command_center.enqueue_command(SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data != None, "Receiving modem did not receive data when it should have."

uav_config_v2 = {
    "name": "test",
    "world": "TestWorld",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "LocationSensor",
                },
                {
                    "sensor_type": "VelocitySensor"
                },
                {
                    "sensor_type": "RGBCamera"
                },
                {
                    "sensor_type": "OpticalModemSensor",
                    "configuration": {
                        "MaxDistance": 3
                    }
                }
            ],
            "control_scheme": 1,
            "location": [0, 0, .5],
            "rotation": [0, 0, 0]
        },
        {
            "agent_name": "uav1",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor",
                    "configuration": {
                        "MaxDistance": 3,
                    }
                }
            ],
            "control_scheme": 1,
            "location": [5, 0, 1],
            "rotation": [0, 0, -180]
        }
    ]
}

def test_within_max_distance():
    "Tests to make sure that two sensors that are not within max distance do not transmit."
    
    binary_path = holodeck.packagemanager.get_binary_path_for_package("DefaultWorlds")


    with holodeck.environments.HolodeckEnvironment(scenario=uav_config_v2,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]

        state, reward, terminal, _ = env.step(command)
        env.agents.get("uav0")._client.command_center.enqueue_command(SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

uav_config_v3 = {
    "name": "test",
    "world": "TestWorld",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "LocationSensor",
                },
                {
                    "sensor_type": "VelocitySensor"
                },
                {
                    "sensor_type": "RGBCamera"
                },
                {
                    "sensor_type": "OpticalModemSensor"
                }
            ],
            "control_scheme": 1,
            "location": [5, 0, .5],
            "rotation": [0, 0, 0]
        },
        {
            "agent_name": "uav1",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor"
                }
            ],
            "control_scheme": 1,
            "location": [0, 0, 1],
            "rotation": [0, 0, -180]
        }
    ]
}
def test_not_oriented():
    "Tests to make sure that two sensors that are not within max distance do not transmit."
    
    binary_path = holodeck.packagemanager.get_binary_path_for_package("DefaultWorlds")


    with holodeck.environments.HolodeckEnvironment(scenario=uav_config_v3,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]

        state, reward, terminal, _ = env.step(command)
        env.agents.get("uav0")._client.command_center.enqueue_command(SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

uav_config_v4 = {
    "name": "test",
    "world": "TestWorld",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "LocationSensor",
                },
                {
                    "sensor_type": "VelocitySensor"
                },
                {
                    "sensor_type": "RGBCamera"
                },
                {
                    "sensor_type": "OpticalModemSensor",
                    "configuration": {
                        "MaxDistance": 10
                    }
                }
            ],
            "control_scheme": 1,
            "location": [-3, 4.5, .25],
            "rotation": [0, 0, -90]
        },
        {
            "agent_name": "uav1",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor",
                    "configuration": {
                        "MaxDistance": 10
                    }
                }
            ],
            "control_scheme": 1,
            "location": [-3, -4.5, .25],
            "rotation": [0, 0, 90]
        }
    ]
}

def test_obstructed_view():
    """Tests to ensure that modem is unable to transmit when there is an obstruction between modems."""
    binary_path = holodeck.packagemanager.get_binary_path_for_package("DefaultWorlds")

    with holodeck.environments.HolodeckEnvironment(scenario=uav_config_v4,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]

        for _ in range(300):
            state, reward, terminal, _ = env.step(command)
            env.agents.get("uav0")._client.command_center.enqueue_command(SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

base_cfg = {
    "name": "test_rgb_camera",
    "world": "TestWorld",
    "main_agent": "sphere0",
    "agents": [
        {
            "agent_name": "sphere0",
            "agent_type": "SphereAgent",
            "sensors": [
                {
                    "sensor_type": "RGBCamera",
                    "socket": "CameraSocket",
                    # note the different camera name. Regression test for #197
                    "sensor_name": "TestCamera"
                }
            ],
            "control_scheme": 0,
            "location": [.95, -1.75, .5] # if you change this, you must change rotation_env too.
        }
    ]
}


def test_rgb_camera(resolution, request):
    """Makes sure that the RGB camera is positioned and capturing correctly.

    Capture pixel data, and load from disk the baseline of what it should look like.
    Then, use mse() to see how different the images are.

    """
    global base_cfg

    cfg = copy.deepcopy(base_cfg)

    cfg["agents"][0]["sensors"][0]["configuration"] = {
        "CaptureWidth": resolution,
        "CaptureHeight": resolution
    }

    binary_path = holodeck.packagemanager.get_binary_path_for_package("DefaultWorlds")

    with holodeck.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:

        for _ in range(5):
            env.tick()

        pixels = env.tick()['TestCamera'][:, :, 0:3]
        baseline = cv2.imread(os.path.join(request.fspath.dirname, "expected", "{}.png".format(resolution)))
        err = mean_square_err(pixels, baseline)

        assert err < 2000
