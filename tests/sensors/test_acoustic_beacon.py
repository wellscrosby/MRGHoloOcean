import holodeck
import uuid
import numpy as np
import pytest


@pytest.fixture(scope="session")
def config():
    scenario = {
        "name": "PerfectAUV",
        "world": "SimpleUnderwater",
        "main_agent": "auv0",
        "agents":[
            {
                "agent_name": "auv0",
                "agent_type": "PerfectHoveringAUV",
                "sensors": [
                    {
                        "sensor_type": "AcousticBeaconSensor",
                        "sensor_name": "Zero",
                        "location": [0,0,0],
                        "configuration": {
                            "id": 0
                        }
                    },
                    {
                        "sensor_type": "AcousticBeaconSensor",
                        "sensor_name": "One",
                        "location": [1,0,0],
                        "configuration": {
                            "id": 1
                        }
                    },
                    {
                        "sensor_type": "AcousticBeaconSensor",
                        "sensor_name": "Two",
                        "location": [0,100,0],
                        "configuration": {
                            "id": 2
                        }
                    }
                ],
                "control_scheme": 0,
                "location": [0.0, 0.0, 5.0],
                "rotation": [0.0, 0.0, 0]
            }
        ],
    }
    return scenario

def test_sending(config):
    """Make sure our sensor rates are working properly
    """
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()

        # send a message
        env.send_acoustic_message(0, 1, "OWAY", "my_message")
        state = env.tick()

        assert 'One' in state
        assert state['One'] == ["OWAY", "my_message"]


@pytest.mark.parametrize('num', range(5))
def test_timing(config, num):
    # do random distance
    dist = np.random.uniform(0, 1000)
    config["agents"][0]["sensors"][2]["location"] = [0, dist, 0]
    num_ticks = int(np.round(dist*30 / (1500)))

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(0, 2, "OWAY", "my_message")
        state = env.tick()


        for _ in range(num_ticks):
            state = env.tick()

        assert "Two" in state
        assert state["Two"] == ["OWAY", "my_message"]


@pytest.mark.parametrize('num', range(5))
def test_distance(config, num):
    # do random distance
    dist = np.random.uniform(0, 1000)
    config["agents"][0]["sensors"][2]["location"] = [0, dist, 0]
    num_ticks = int(np.round(dist*30 / (1500)))

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(0, 2, "MSG_REQU", "my_message")
        state = env.tick()
        for _ in range(num_ticks):
            state = env.tick()

        # get it back
        state = env.tick()
        for _ in range(num_ticks):
            state = env.tick()

        assert "Zero" in state
        # TODO determine what we want it to send back in these scenarios
        assert state["Zero"][0:2] == ["MSG_RESPU", None]
        assert np.isclose(state["Zero"][4], dist)


def test_all_to_one(config):
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(0, 2, "OWAY", "my_message")
        state = env.tick()
        env.send_acoustic_message(1, 2, "OWAY", "my_message")

        for _ in range(10):
            state = env.tick()
            assert "Two" not in state

        assert env.beacons_status == ["Idle"]*3


def test_one_to_all(config):
    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(0, -1, "OWAY", "my_message")

        one = False
        two = False
        for _ in range(10):
            state = env.tick()
            
            one = one or "One" in state
            two = two or "Two" in state

            if one and two:
                break
            else:
                assert env.beacons_status[0] == "Transmitting"

        assert one
        assert two
        assert env.beacons_status == ["Idle"]*3

@pytest.mark.parametrize('data', zip([[1,0],[0,1],[-1,0],[0,-1],[1,1]],
                                    [0, np.pi/2, np.pi, -np.pi/2, np.pi/4]))
def test_azimuth(config, data):
    xy, angle = data
    config["agents"][0]["sensors"][2]["location"] = [xy[0], xy[1], 0]

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(2, 0, "OWAYU", "my_message")
        state = env.tick()

        assert np.isclose(state['Zero'][2], angle)


@pytest.mark.parametrize('data', zip([[1,0], [1,1], [1,-1], [-1,-1]],
                                    [0, np.pi/4, -np.pi/4, -np.pi/4]))
def test_elevation(config, data):
    yz, angle = data
    config["agents"][0]["sensors"][2]["location"] = [0, yz[0], yz[1]]

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4()),
                                                   ticks_per_sec=30) as env:
        env.reset()
        
        # send a message
        env.send_acoustic_message(2, 0, "OWAYU", "my_message")
        state = env.tick()

        assert np.isclose(state['Zero'][3], angle)