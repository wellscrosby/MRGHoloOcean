import holodeck
#from holodeck import sensors
#import holodeck.command
import uuid
import copy

uav_config_v1 = {
    "name": "test",
    "world": "Rooms",
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
            "location": [-10, 10, .25],
            "rotation": [0, 0, -90]
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
            "location": [-10, 7, .25],
            "rotation": [0, 0, 90]
        }
    ]
}

def test_transmittable():
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    global uav_config_v1
    cfg = copy.deepcopy(uav_config_v1)

    with holodeck.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]
        for _ in range (20):
            state, reward, terminal, _ = env.step(command)
            env.agents.get("uav0")._client.command_center.enqueue_command(holodeck.command.SendOpticalMessageCommand("uav0", "OpticalModemSensor","uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data != None, "Receiving modem did not receive data when it should have."

uav_config_v2 = {
    "name": "test",
    "world": "Rooms",
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
                        "MaxDistance": 3,
                    }
                }
            ],
            "control_scheme": 1,
            "location": [-5, 10, .25],
            "rotation": [0, 0, -90]
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
            "location": [-5, 6.5, .25],
            "rotation": [0, 0, 90]
        }
    ]
}

def test_within_max_distance():
    "Tests to make sure that two sensors that are not within max distance do not transmit."
    
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v2
    cfg = copy.deepcopy(uav_config_v2)



    with holodeck.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]
        state, reward, terminal, _ = env.step(command)
        for i in range (20):
            env.tick()
            env.agents.get("uav0")._client.command_center.enqueue_command(holodeck.command.SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

uav_config_v3 = {
    "name": "test",
    "world": "Rooms",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor"
                }
            ],
            "control_scheme": 1,
            "location": [-10, 10, .25],
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
            "location": [-10, 7, .25],
            "rotation": [0, 0, -180]
        }
    ]
}
def test_not_oriented():
    "Tests to make sure that two sensors that are not within max distance do not transmit."
    
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v3
    cfg = copy.deepcopy(uav_config_v3)

    with holodeck.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]
        for i in range (20):
            state, reward, terminal, _ = env.step(command)
            env.agents.get("uav0")._client.command_center.enqueue_command(holodeck.command.SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

uav_config_v4 = {
    "name": "test",
    "world": "Rooms",
    "main_agent": "uav0",
    "agents": [
        {
            "agent_name": "uav0",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor",
                }
            ],
            "control_scheme": 1,
            "location": [-10, 10, .25],
            "rotation": [0, 0, -90]
        },
        {
            "agent_name": "uav1",
            "agent_type": "UavAgent",
            "sensors": [
                {
                    "sensor_type": "OpticalModemSensor",
                }
            ],
            "control_scheme": 1,
            "location": [-10, 2, .25],
            "rotation": [0, 0, -180]
        }
    ]
}

def test_obstructed_view():
    """Tests to ensure that modem is unable to transmit when there is an obstruction between modems."""
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v4
    cfg = copy.deepcopy(uav_config_v4)

    with holodeck.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        command = [0, 0, 10, 50]

        for _ in range(300):
            state, reward, terminal, _ = env.step(command)
            env.agents.get("uav0")._client.command_center.enqueue_command(holodeck.command.SendOpticalMessageCommand("uav0","OpticalModemSensor", "uav1", "OpticalModemSensor"))

        assert env.agents.get("uav1").sensors.get("OpticalModemSensor").sensor_data == None, "Receiving modem received data when it should not have done so."

        if __name__ == "__main__":
            test_transmittable()