.. _`surface-vessel-agent`:

SurfaceVessel
===============

Images
------

.. .. image:: images/hovering-auv.png
..    :scale: 40%

Description
-----------
A simple surface vessel with 2 thrusters for propolsion.

See the :class:`~holoocean.agents.SurfaceVessel`.

Control Schemes
---------------

**Thrusters (``0``)**
  An 2-length floating point vector used to specify the control on each thruster. First one is left, second is right.

**PD Controller (``1``)**
   A 2-length floating point vector of desired x and y position in the global frame. A basic PD controller has been implemented to move the vehicle to that position using the correct thruster forces.

**Custom Dynamics (``2``)**
   A 6-length floating point vector of linear and angular accelerations in the global frame. This is to be used for implementing custom dynamics. Besides collisions, all other forces and torques - including gravity, buoyancy, and damping - have been disabled in the simulator to allow for a clean slate for custom dynamics.

Sockets
-------

.. - ``COM`` Center of mass
.. - ``DVLSocket`` Location of the DVL
.. - ``IMUSocket`` Location of the IMU.
.. - ``DepthSocket`` Location of the depth sensor.
.. - ``SonarSocket`` Location of the sonar sensor.
.. - ``CameraRightSocket`` Location of the left camera.
.. - ``CameraLeftSocket`` Location of the right camera.
.. - ``Origin`` true center of the robot
.. - ``Viewport`` where the robot is viewed from.

.. .. image:: images/hovering-angled.png
..    :scale: 50%

.. .. image:: images/hovering-top.png
..    :scale: 50%

.. .. image:: images/hovering-right.png
..    :scale: 50%

.. .. image:: images/hovering-front.png
..    :scale: 50%