import holoocean
import numpy as np
from tqdm import tqdm

config = {
        "name": "test",
        "world": "OpenWater",
        "package_name": "Ocean",
        "main_agent": "auv0",
        "agents": [
            {
                "agent_name": "auv0",
                "agent_type": "UavAgent",
                "sensors": [
                    {
                        "sensor_type": "LocationSensor",
                    },
                    {
                        "sensor_type": "VelocitySensor"
                    },
                ],
                "control_scheme": 0,
                "location": [0, 0, 3]
            }
        ]
    }

command = np.zeros(8)
with holoocean.make("OpenWater-HoveringCamera", show_viewport=False, verbose=True) as env:
    # try:
        # while True:
        for i in tqdm(range(100)):
            env.act("auv0", command)
            state = env.tick()
            
    # except:
        # print("Closing...")
