# Holodeck

![Holodeck Image](docs/images/inspect_plane.jpg)

[![Documentation Status](https://readthedocs.org/projects/holoocean/badge/?version=latest)](https://holoocean.readthedocs.io/en/latest/?badge=latest)
 [![Build Status](https://robots.et.byu.edu:4000/api/badges/frostlab/holoocean/status.svg?ref=refs/heads/develop)](https://robots.et.byu.edu:4000/frostlab/holoocean)


Holodeck is a high-fidelity simulator for reinforcement learning built on top of Unreal Engine 4.

## Features
 - 7+ rich worlds for training agents in, and many scenarios for those worlds
 - Linux and Windows support
 - Easily extend and modify training scenarios
 - Train and control more than one agent at once
 - Simple, OpenAI Gym-like Python interface
 - High performance - simulation speeds of up to 2x real time are possible. Performance penalty only for what you need
 - Run headless or watch your agents learn

### Questions? [Join our Discord!](https://discord.gg/Xqqksje)

## Installation
`pip install holodeck`

(requires >= Python 3.5)

See [Installation](https://holoocean.readthedocs.io/en/latest/usage/installation.html) for complete instructions (including Docker).

## Documentation
* [Quickstart](https://holoocean.readthedocs.io/en/latest/usage/getting-started.html)
* [Changelog](https://holoocean.readthedocs.io/en/latest/changelog/changelog.html)
* [Examples](https://holoocean.readthedocs.io/en/latest/usage/getting-started.html#code-examples)
* [Agents](https://holoocean.readthedocs.io/en/latest/agents/agents.html)
* [Sensors](https://holoocean.readthedocs.io/en/latest/holodeck/sensors.html)
* [Available Packages and Worlds](https://holoocean.readthedocs.io/en/latest/packages/packages.html)
* [Docs](https://holoocean.readthedocs.io/en/latest/)

## Usage Overview
Holodeck's interface is similar to [OpenAI's Gym](https://gym.openai.com/). 

We try and provide a batteries included approach to let you jump right into using Holodeck, with minimal
fiddling required.

To demonstrate, here is a quick example using the `DefaultWorlds` package:

```python
import holoocean

# Load the environment. This environment contains a UAV in a city.
env = holoocean.make("UrbanCity-MaxDistance")

# You must call `.reset()` on a newly created environment before ticking/stepping it
env.reset()                         

# The UAV takes 3 torques and a thrust as a command.
command = [0, 0, 0, 100]   

for i in range(30):
    state, reward, terminal, info = env.step(command)  
```

- `state`: dict of sensor name to the sensor's value (nparray).
- `reward`: the reward received from the previous action
- `terminal`: indicates whether the current state is a terminal state.
- `info`: contains additional environment specific information.

If you want to access the data of a specific sensor, import sensors and
retrieving the correct value from the state dictionary:

```python
print(state["LocationSensor"])
```

## Multi Agent-Environments
Holodeck supports multi-agent environments.

Calls to [`step`](https://holoocean.readthedocs.io/en/latest/holodeck/environments.html#holoocean.environments.HolodeckEnvironment.step) only provide an action for the main agent, and then tick the simulation. 

[`act`](https://holoocean.readthedocs.io/en/latest/holodeck/environments.html#holoocean.environments.HolodeckEnvironment.act) provides a persistent action for a specific agent, and does not tick the simulation. After an 
action has been provided, [`tick`](https://holoocean.readthedocs.io/en/latest/holodeck/environments.html#holoocean.environments.HolodeckEnvironment.tick) will advance the simulation forward. The action is persisted until another call to `act` provides a different action.

```python
import holoocean
import numpy as np

env = holoocean.make("CyberPunkCity-Follow")
env.reset()

# Provide an action for each agent
env.act('uav0', np.array([0, 0, 0, 100]))
env.act('nav0', np.array([0, 0, 0]))

# Advance the simulation
for i in range(300):
  # The action provided above is repeated
  states = env.tick()
```

You can access the reward, terminal and location for a multi agent environment as follows:

```python
task = states["uav0"]["FollowTask"]

reward = task[0]
terminal = task[1]
location = states["uav0"]["LocationSensor"]
```

(`uav0` comes from the [scenario configuration file](https://holoocean.readthedocs.io/en/latest/packages/docs/scenarios.html))

## Running Holodeck Headless
Holodeck can run headless with GPU accelerated rendering. See [Using Holodeck Headless](https://holoocean.readthedocs.io/en/latest/usage/running-headless.html)

## Citation:
```
@misc{HolodeckPCCL,
  Author = {Joshua Greaves and Max Robinson and Nick Walton and Mitchell Mortensen and Robert Pottorff and Connor Christopherson and Derek Hancock and Jayden Milne and David Wingate},
  Title = {Holodeck: A High Fidelity Simulator},
  Year = {2018},
}
```

Holodeck is a project of BYU's Perception, Cognition and Control Lab (https://pcc.cs.byu.edu/).
