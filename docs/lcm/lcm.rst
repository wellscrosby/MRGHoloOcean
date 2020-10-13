Publishing Sensor Data
=========================

Currently, Holodeck-Ocean supports publishing data to LCM (with a potential ROS wrapper being considered). All this config happens in the :ref:`scenario <scenarios>` file. We'll outline what this takes here.

LCM publishes data to a certain medium, called the provider. This can be locally, over the network, a log file, etc. This can be specified in the header of the
scenario file. See _here for options on this.

.. _here: https://lcm-proj.github.io/group__LcmC__lcm__t.html#gabb730c9e49442a4bcf400e0f2fef7576

.. code-block:: json

   {
      "name": "{Scenario Name}",
      "world": "{world it is associated with}",
      "lcm_provider": "file:///home/lcm.log"
      "agents":[
         "array of agent objects"
      ]
   }

Once the provider is chosen, Holodeck publishes each sensor individually. The publisher (currently LCM is only option) is set
by the sensor config. Along with this is a channel option, that sets the channel to publish data to.

.. code-block:: json

   {
      "sensor_type": "RGBCamera",
      "sensor_name": "FrontCamera",
      "publish": "lcm",
      "channel": "CAMERA",
   }

The ``publish`` parameter defaults to blank (no publishing) and ``channel`` will default to ``sensor_type``.