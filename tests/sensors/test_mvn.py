import holodeck
import pytest
import uuid

from scipy.stats import multivariate_normal as mvn
import numpy as np

eps = 1e-7

@pytest.fixture
def config():
    config = {
        "name": "test_mvn",
        "world": "Rooms",
        "main_agent": "turtle0",
        "agents": [
            {
                "agent_name": "turtle0",
                "agent_type": "TurtleAgent",
                "sensors": [
                    {
                        "sensor_type": "LocationSensor",
                        "sensor_name": "noise",
                        "configuration": {}
                    },
                    {
                        "sensor_type": "LocationSensor",
                        "sensor_name": "clean"
                    }
                ],
                "control_scheme": 0,
                "location": [-1.5, -1.50, 3.0]
            }
        ]
    }
    return config


@pytest.mark.parametrize('num', range(5))
def test_sigma_scalar(config, num):
    """Make sure it can take a plain std as a parameter
    """
    # Generate a sigma
    sigma = np.random.rand()*10
    cov = np.diag([sigma, sigma, sigma])**2

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    config['agents'][0]['sensors'][0]['configuration']['Sigma'] = sigma

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        
        for i in range(5):
            state = env.tick()

            p = mvn.pdf(state['noise'], mean=state['clean'], cov=cov)
            assert p > eps

@pytest.mark.parametrize('num', range(5))
def test_sigma_vector(config, num):
    """Make sure it can take diagonal array as std
    """
    # Generate a sigma
    sigma = np.random.rand(3)*10
    cov = np.diag(sigma**2)

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    config['agents'][0]['sensors'][0]['configuration']['Sigma'] = list(sigma)

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        
        for i in range(5):
            state = env.tick()

            p = mvn.pdf(state['noise'], mean=state['clean'], cov=cov)
            assert p > eps

@pytest.mark.parametrize('num', range(5))
def test_cov_num(config, num):
    """Make sure it can take covariance as a scalar
    """
    # Generate a sigma
    cov_num = np.random.rand()*10
    cov = np.diag([cov_num, cov_num, cov_num])

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    config['agents'][0]['sensors'][0]['configuration']['Cov'] = cov_num

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        
        for i in range(5):
            state = env.tick()

            p = mvn.pdf(state['noise'], mean=state['clean'], cov=cov)
            assert p > eps

@pytest.mark.parametrize('num', range(5))
def test_cov_vector(config, num):
    """Make sure it can take diagonal array as cov
    """
    # Generate a cov
    cov_array = np.random.rand(3)*10
    cov = np.diag(cov_array)

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    config['agents'][0]['sensors'][0]['configuration']['Cov'] = list(cov_array)

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        
        for i in range(5):
            state = env.tick()

            p = mvn.pdf(state['noise'], mean=state['clean'], cov=cov)
            assert p > eps

@pytest.mark.parametrize('num', range(5))
def test_cov_matrix(config, num):
    """Make sure it can take a full matrix as covariance
    """
    # Generate a cov
    cov = np.random.rand(3,3)*10
    cov = cov@cov.T

    binary_path = holodeck.packagemanager.get_binary_path_for_package("Ocean")
    config['agents'][0]['sensors'][0]['configuration']['Cov'] = cov.tolist()

    with holodeck.environments.HolodeckEnvironment(scenario=config,
                                                   binary_path=binary_path,
                                                   show_viewport=False,
                                                   uuid=str(uuid.uuid4())) as env:
        
        for i in range(5):
            state = env.tick()

            p = mvn.pdf(state['noise'], mean=state['clean'], cov=cov)
            assert p > eps

