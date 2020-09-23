from holodeck.lcm import DVLSensor, IMUSensor, LocationSensor, RangeFinderSensor, RotationSensor, VelocitySensor
import os

class SensorData:
    _sensor_keys_ = {
        "IMUSensor": IMUSensor,
        "DVLSensor": DVLSensor,
        "LocationSensor": LocationSensor,
        "RangeFinderSensor": RangeFinderSensor,
        "RotationSensor": RotationSensor,
        "VelocitySensor": VelocitySensor,
    }

    def __init__(self, sensor_type, channel):
        self.type = sensor_type
        self.sensor = self._sensor_keys_[sensor_type]()
        self.channel = channel

    def set_value(self, timestamp, value):
        self.sensor.timestamp = timestamp

        if self.type == "IMUSensor":
            self.sensor.acceleration = value[0].tolist()
            self.sensor.angular_velocity = value[1].tolist()
        elif self.type == "DVLSensor":
            self.sensor.velocity = value.tolist()
        elif self.type == "LocationSensor":
            self.sensor.position = value.tolist()
        elif self.type == "RangeFinderSensor":
            self.sensor.count = len(value)
            self.sensor.distances = value.tolist()
        elif self.type == "RotationSensor":
            self.sensor.roll, self.sensor.pitch, self.sensor.yaw = value
        elif self.type == "VelocitySensor":
            self.sensor.velocity = value.tolist()
        else:
            raise ValueError("That sensor hasn't been implemented in LCM yet")

def gen(lang, path='.', headers=None):
    """Generates LCM files for sensors in whatever language requested. 

    Args:
        lang (:obj: `str`): One of "cpp", "c", "java", "python", "lua", "csharp", "go"
        path (:obj:`str`, optional): Location to save files in. Defaults to current directory.
        headers (:obj: `str`, optional): Where to store .h files for C . Defaults to same as c files, given by path arg.
    """
    #set up all paths
    lcm_path = os.path.join( os.path.dirname(os.path.realpath(__file__)), 'sensors.lcm' )
    path = os.path.abspath(path)
    if headers is None:
        headers = path
    else:
        headers = os.path.abspath(headers)

    #determine flags to send with it
    if lang == "c":
        flags = f"-c --c-cpath {path} --c-hpath {headers}"
    elif lang == "cpp":
        flags = f"-x --cpp-hpath {path}"
    elif lang == "java":
        flags = f"-j --jpath {path}"
    elif lang == "python":
        flags = f"-p --ppath {path}"
    elif lang == "lua":
        flags = f"-l --lpath {path}"
    elif lang == "csharp":
        flags = f"--csharp --csharp-path {path}"
    elif lang == "go":
        flags = f"--go --go-path {path}"
    else:
        raise ValueError("Not a valid language for LCM files.")
    
    #run the command
    os.system(f"lcm-gen {lcm_path} {flags}")