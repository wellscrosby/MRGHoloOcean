import holodeck
import uuid
import copy from deepcopy
from holodeck.command import SendOpticalMessageCommand
from holodeck import sensors

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