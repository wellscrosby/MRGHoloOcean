.. _scenarios:

Scenarios
===================

What is a scenario?
-------------------

A scenario tells HoloOcean which world to load, which agents to place in the
world, and which sensors they need.

It defines:

- Which world to load
- Agent Definitions

  - What type of agent they are
  - Where they are
  - What sensors they have

.. tip::
   You can think of scenarios like a map or gametype variant from Halo:
   the world or map itself doesn't change, but the things in the world
   and your objective can change.

Scenarios allow the same world to be used for many different purposes,
and allows you to extend and customize the scenarios we provide to
suit your needs without repackaging the engine.

When you call ``holoocean.make()`` to create an environment, you pass in the
name of a scenario, eg ``holoocean.make("Pier-Hovering")``. This tells
HoloOcean which world to load and where to place agents.

.. _`scenario-files`:

Scenario File Format
--------------------

Scenario ``.json`` files are distributed in packages (see
:ref:`package-contents`), and must be named
``{WorldName}-{ScenarioName}.json``. By default they are stored in the
``worlds/{PackageName}`` directory, but they can be loaded from a
Python dictionary as well.

Scenario File
~~~~~~~~~~~~~

.. code-block:: json

   {
      "name": "{Scenario Name}",
      "world": "{world it is associated with}",
      "lcm_provider": "{Optional, where to publish lcm to}",
      "env_min": [-10, -10, -10],
      "env_max": [10, 10, 10],
      "octree_min": 0.1,
      "octree_max": 5,
      "agents":[
         "array of agent objects"
      ],
      "weather": {
         "hour": 12,
         "type": "'sunny' or 'cloudy' or 'rain'",
         "fog_density": 0,
         "day_cycle_length": 86400
      },
      "window_width":  1280,
      "window_height": 720
   }

``window_width/height`` control the size of the window opened when an
environment is created. For more information about weather options, see
:ref:`weather`.

.. note::
   The first agent in the ``agents`` array is the "main agent"

.. _`configure-octree`:

Configuring Octree
~~~~~~~~~~~~~~~~~~

When using a form of sonar sensor and initializing the world, an Octree will either be
created or loaded from a cache. The parameters of these can be set using the ``env_min``,
``env_max``, ``octree_min``, and ``octree_max``. The octrees are cached in the ``LinuxNoEditor/Holodeck/Octrees`` folder
in the worlds folder. See :ref:`package-locations`.

``env_min``/``env_max`` are used to set the upper/lower bounds of the environment. They should 
be set in :ref:`package-structure`, but the values set here will override it.

``octree_min``/``octree_max`` are used to set the minimum/mid-level size of the octree. ``octree_min``
can go as low as .01 (1cm), and then the octree will double in size till it reaches ``octree_max``.



Agent objects
~~~~~~~~~~~~~

.. code-block:: json

   {
      "agent_name": "uav0",
      "agent_type": "{agent types}",
      "sensors": [
         "array of sensor objects"
      ],
      "control_scheme": "{control scheme type}",
      "location": [1.0, 2.0, 3.0],
      "rotation": [1.0, 2.0, 3.0],
      "location_randomization": [1, 2, 3],
      "rotation_randomization": [10, 10, 10]
   }

.. note::
   HoloOcean coordinates are **right handed** in meters. See :ref:`coordinate-system`

.. _`location-randomization`:

Location Randomization
**********************

``location_randomization`` and ``rotation_randomization`` are optional. If
provided, the agent's start location and/or rotation will vary by a
random amount between the negative and the positive values of the
provided randomization values as sampled from a uniform distribution.

The location value is measured in meters, in the format ``[dx, dy, dz]``
and the rotation is ``[roll, pitch, yaw]``.

Agent Types
***********

Here are valid ``agent_type`` s:

========================= ========================
Agent Type                String in agent_type
========================= ========================
:ref:`hovering-auv-agent`  ``HoveringAUV``
:ref:`Torpedo-auv-agent`   ``TorpedoAUV``
:ref:`turtle-agent`        ``TurtleAgent``
:ref:`uav-agent`           ``UAV``
========================= ========================

Control Schemes
***************

Control schemes are represented as an integer. For valid values and a
description of how each scheme works, see the documentation pages for each
agent.

Sensor Objects
~~~~~~~~~~~~~~

.. code-block:: json

   {
      "sensor_type": "RGBCamera",
      "sensor_name": "FrontCamera",
      "location": [1.0, 2.0, 3.0],
      "rotation": [1.0, 2.0, 3.0],
      "socket": "socket name or \"\"",
      "lcm_channel": "channel_name",
      "configuration": {

      }
   }

Sensors have a couple options for placement.

1. **Provide a socket name**

   This will place the sensor in the given socket

   .. code-block:: json

      {
         "sensor_type": "RGBCamera",
         "socket": "CameraSocket"
      }

2. **Provide a socket and a location/rotation**

   The sensor will be placed offset to the socket by the location and rotation


   .. code-block:: json

      {
         "sensor_type": "RGBCamera",
         "location": [1.0, 2.0, 3.0],
         "socket": "CameraSocket"
      }

3. **Provide just a location/rotation**

   The sensor will be placed at the given coordinates, offset from the root of
   the agent.

   .. code-block:: json

      {
         "sensor_type": "RGBCamera",
         "location": [1.0, 2.0, 3.0]
      }

4 **Publish Message**

   Currently, HoloOcean supports publishing mesages to LCM (with possible ROS package coming).
   To publish sensor data to LCM, specify the type to publish.

   .. code-block:: json

      {
         "sensor_type": "RGBCamera",
         "lcm_channel": "CAMERA"
      }

   The channel parameter specifies which channel to publish the sensor data to.

The only keys that are required in a sensor object is ``"sensor_type"``, the
rest will default as shown below

.. code-block:: json

   {
      "sensor_name": "sensor_type",
      "location": [0, 0, 0],
      "rotation": [0, 0, 0],
      "socket": "",
      "publish": "",
      "lcm_channel": "",
      "configuration": {}
   }

.. _`configuration-block`:

Configuration Block
~~~~~~~~~~~~~~~~~~~

The contents of the ``configuration`` block are sensor-specific. That block is
passed verbatim to the sensor itself, which parses it.

For example, the docstring for :class:`~holoocean.sensors.RGBCamera` states that
it accepts ``CaptureWidth`` and ``CaptureHeight`` parameters, so an example
sensor configuration would be:

.. code-block:: json

   {
      "sensor_name": "RBGCamera",
      "socket": "CameraSocket",
      "configuration": {
         "CaptureHeight": 1920,
         "CaptureWidth": 1080
      }
   }
