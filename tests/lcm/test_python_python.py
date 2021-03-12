import holodeck
import uuid
import pytest

@pytest.mark.parametrize("sensor", ["DVLSensor", "IMUSensor", "LocationSensor", 
                        "RangeFinderSensor", "RotationSensor", "VelocitySensor", "OrientationSensor", "PoseSensor"])
def test_sensor(sensor):
    config = {
                "name": "test",
                "world": "Rooms",
                "main_agent": "turtle0",
                "lcm_provider": "memq://",
                "agents": [
                    {
                        "agent_name": "turtle0",
                        "agent_type": "TurtleAgent",
                        "sensors": [
                            {
                                "sensor_type": sensor,
                                "publish": "lcm",
                                "lcm_channel": "sensor"
                            }
                        ],
                        "control_scheme": 0,
                        "location": [-1.5, 1.50, 3.0]
                    }
                ]
            }

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    d = {"i" : 0}

    def my_handler(channel, data):
        d['i'] += 1

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        sub = env._lcm.subscribe("sensor", my_handler)

        for _ in range(100):
            env.tick()
            env._lcm.handle()

        assert d['i'] == 100, f"LCM only received {i} of 100 messages"

        env._lcm.unsubscribe(sub)