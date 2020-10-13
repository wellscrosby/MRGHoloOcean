import holodeck
import uuid

turtle_config = {
    "name": "test_velocity_sensor",
    "world": "Rooms",
    "main_agent": "turtle0",
    "agents": [
        {
            "agent_name": "turtle0",
            "agent_type": "TurtleAgent",
            "sensors": [
                {
                    "sensor_type": "DVLSensor",
                }
            ],
            "control_scheme": 0,
            "location": [-1.5, 1.50, 3.0]
        }
    ]
}


def test_dvl_sensor_straight():
    """Make sure when we move forward x is positive, y is close to 0. Make sure when not moving both are close to 0
    """

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    turtle_config['agents'][0]['rotation'] = [0, 0, 0]

    with holodeck.environments.HolodeckEnvironment(scenario=turtle_config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        #let it land and then start moving forward
        for _ in range(100):
            last_x_velocity, _ = env.tick()["DVLSensor"]
        env.step([150,0])

        #Move forward, making sure y is relatively small, and x is increasing
        for i in range(50):
            new_x_velocity, y_velocity = env.step([150,0])[0]["DVLSensor"]
            assert new_x_velocity >= last_x_velocity, f"The velocity didn't increase at step {i}!"
            assert y_velocity <= .5
            last_x_velocity = new_x_velocity

        #Let it stop
        for _ in range(100):
            env.step([0, 0])

        #Move backward, making sure y is relatively small, and x is decreasing
        for i in range(50):
            new_x_velocity, y_velocity = env.step([-150,0])[0]["DVLSensor"]
            assert new_x_velocity <= last_x_velocity, f"The velocity didn't decrease at step {i}!"
            assert y_velocity <= .5
            last_x_velocity = new_x_velocity

        #Let it stop
        for _ in range(100):
            env.step([0, 0])

        #make sure everythign is close to 0
        x_velocity, y_velocity = env.tick()["DVLSensor"]
        assert x_velocity <= 1e-2, "The x velocity wasn't close enough to zero!"
        assert y_velocity <= 1e-2, "The y velocity wasn't close enough to zero!"

def test_dvl_sensor_rotated():
    """Make sure when we move forward x is positive, y is close to 0. Make sure when not moving both are close to 0
    """

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    turtle_config['agents'][0]['rotation'] = [-90, 0, 0]

    with holodeck.environments.HolodeckEnvironment(scenario=turtle_config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        #let it land and then start moving forward
        for _ in range(100):
            last_x_velocity, _ = env.tick()["DVLSensor"]
        env.step([150,0])

        #Move forward, making sure y is relatively small, and x is increasing
        for i in range(60):
            new_x_velocity, y_velocity = env.step([150,0])[0]["DVLSensor"]
            assert new_x_velocity >= last_x_velocity, f"The velocity didn't increase at step {i}!"
            assert y_velocity <= .5
            last_x_velocity = new_x_velocity

        #Let it stop
        for _ in range(100):
            env.step([0, 0])

        #Move backward, making sure y is relatively small, and x is decreasing
        for i in range(20):
            new_x_velocity, y_velocity = env.step([-150,0])[0]["DVLSensor"]
            assert new_x_velocity <= last_x_velocity, f"The velocity didn't decrease at step {i}!"
            assert y_velocity <= .5
            last_x_velocity = new_x_velocity

        #Let it stop
        for _ in range(100):
            env.step([0, 0])

        #make sure everythign is close to 0
        x_velocity, y_velocity = env.tick()["DVLSensor"]
        assert x_velocity <= 1e-2, "The x velocity wasn't close enough to zero!"
        assert y_velocity <= 1e-2, "The y velocity wasn't close enough to zero!"