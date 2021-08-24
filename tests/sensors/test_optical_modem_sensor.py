import holoocean
import uuid
import copy

from numpy import equal

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
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")

    global uav_config_v1
    cfg = copy.deepcopy(uav_config_v1)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()

        command = [0, 0, 10, 50]
        for _ in range (20):
            env.send_optical_message(0, 1, "Message")
            state, reward, terminal, _ = env.step(command)

        assert "OpticalModemSensor" in state["uav1"], "Receiving modem did not receive data when it should have."
        assert state["uav1"]["OpticalModemSensor"] == "Message", "Wrong message received."

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
            "location": [-5, 3, .25],
            "rotation": [0, 0, 90]
        }
    ]
}

def test_within_max_distance():
    "Tests to make sure that two sensors that are not within max distance do not transmit."
    
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v2
    cfg = copy.deepcopy(uav_config_v2)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()

        command = [0, 0, 10, 50]
        state, reward, terminal, _ = env.step(command)
        for i in range (20):
            env.send_optical_message(0, 1, "Message")
            env.tick()
        assert "OpticalModemSensor" not in state["uav1"], "Receiving modem received data when it should not have done so."

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
    
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v3
    cfg = copy.deepcopy(uav_config_v3)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()

        command = [0, 0, 10, 50]
        for i in range (20):
            env.send_optical_message(0, 1, "Message")
            state, reward, terminal, _ = env.step(command)
        assert "OpticalModemSensor" not in state["uav1"], "Receiving modem received data when it should not have done so."

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
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v4
    cfg = copy.deepcopy(uav_config_v4)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()
        
        command = [0, 0, 10, 50]

        for _ in range(300):
            env.send_optical_message(0, 1, "Message")
            state, reward, terminal, _ = env.step(command)
        assert "OpticalModemSensor" not in state["uav1"], "Receiving modem received data when it should not have done so."

uav_config_v5 = {
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
                    "configuration": {
                        "MaxDistance": 3,
                        "DistanceSigma": 1
                    }
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

def test_distance_noise():
    """Tests to ensure that noise generation for max distance is functional"""
    num_tests = 100
    tests_passed = 0
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v5
    cfg = copy.deepcopy(uav_config_v5)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()

        command = [0, 0, 10, 50]
        for _ in range (num_tests):
            env.send_optical_message(0, 1, "Message")
            state, reward, terminal, _ = env.step(command)
            print(state)
            if "OpticalModemSensor" in state["uav1"] and state["uav1"]["OpticalModemSensor"] == "Message":
                tests_passed += 1

        assert tests_passed < num_tests, "All messages sent when some should have failed due to noise variation."
        assert tests_passed > 0, "All messages failed when some should have passed due to noise variation."

uav_config_v6 = {
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
                    "configuration": {
                        "MaxDistance": 6,
                        "AngleSigma": 20
                    }
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
                    "configuration": {
                        "MaxDistance": 6,
                        "AngleSigma": 20
                    }
                }
            ],
            "control_scheme": 1,
            "location": [-15.2, 7, .25],
            "rotation": [0, 0, 90]
        }
    ]
}

def test_angle_noise():
    """Tests to ensure that noise generation for max distance is functional"""
    num_tests = 100
    tests_passed = 0
    binary_path = holoocean.packagemanager.get_binary_path_for_package("Ocean")
    global uav_config_v5
    cfg = copy.deepcopy(uav_config_v5)

    with holoocean.environments.HolodeckEnvironment(scenario=cfg,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        env.reset()

        command = [0, 0, 10, 50]
        for _ in range (num_tests):
            env.send_optical_message(0, 1, "Message")
            state, reward, terminal, _ = env.step(command)
            print(state)
            if "OpticalModemSensor" in state["uav1"] and state["uav1"]["OpticalModemSensor"] == "Message":
                tests_passed += 1

        assert tests_passed < num_tests, "All messages sent when some should have failed due to noise variation."
        assert tests_passed > 0, "All messages failed when some should have passed due to noise variation."
