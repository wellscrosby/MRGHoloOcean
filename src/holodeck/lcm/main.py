from holodeck.lcm import DVLSensor, IMUSensor

class SensorData:
    _sensor_keys_ = {
        "IMUSensor": IMUSensor,
        "DVLSensor": DVLSensor,
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
        if self.type == "DVLSensor":
            self.sensor.velocity = value.tolist()