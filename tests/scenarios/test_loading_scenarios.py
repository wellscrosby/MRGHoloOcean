import holoocean


def test_load_scenario(scenario):
    """Tests that every scenario can be loaded without any errors

    TODO: We need some way of communicating with the engine to verify that the expected level was loaded.
          If the level isn't found, then Unreal just picks a default one, so we're missing that case

    Args:
        scenario (str): Scenario to test

    """
    env = holoocean.make(scenario, show_viewport=False, frames_per_sec=False)
    for _ in range(30):
        env.tick()
    env.__on_exit__()


def test_all_agents_and_sensors_present(env_scenario):
    """Test that every agent has every sensor that is present in the config file

    Args:
        env_scenario ((HoloOceanEnvironment, str)): environment and scenario we are testing

    """
    env, scenario = env_scenario
    scenario = holoocean.packagemanager.get_scenario(scenario)

    assert len(env.agents) == len(scenario['agents']), \
        "Length of agents did not match!"

    for agent in scenario['agents']:
        assert agent['agent_name'] in env.agents, \
            "Agent is not in the environment!"

        assert len(agent['sensors']) == len(env.agents[agent['agent_name']].sensors), \
            "length of sensors did not match!"

        for sensor in agent['sensors']:
            assert sensor['sensor_type'] in env.agents[agent['agent_name']].sensors, \
                "Sensor is missing!"

