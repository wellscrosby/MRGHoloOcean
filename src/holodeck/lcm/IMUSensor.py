"""LCM type definitions
This file automatically generated by lcm.
DO NOT MODIFY BY HAND!!!!
"""

try:
    import cStringIO.StringIO as BytesIO
except ImportError:
    from io import BytesIO
import struct

class IMUSensor(object):
    __slots__ = ["timestamp", "acceleration", "angular_velocity"]

    __typenames__ = ["int64_t", "float", "float"]

    __dimensions__ = [None, [3], [3]]

    def __init__(self):
        self.timestamp = 0
        self.acceleration = [ 0.0 for dim0 in range(3) ]
        self.angular_velocity = [ 0.0 for dim0 in range(3) ]

    def encode(self):
        buf = BytesIO()
        buf.write(IMUSensor._get_packed_fingerprint())
        self._encode_one(buf)
        return buf.getvalue()

    def _encode_one(self, buf):
        buf.write(struct.pack(">q", self.timestamp))
        buf.write(struct.pack('>3f', *self.acceleration[:3]))
        buf.write(struct.pack('>3f', *self.angular_velocity[:3]))

    def decode(data):
        if hasattr(data, 'read'):
            buf = data
        else:
            buf = BytesIO(data)
        if buf.read(8) != IMUSensor._get_packed_fingerprint():
            raise ValueError("Decode error")
        return IMUSensor._decode_one(buf)
    decode = staticmethod(decode)

    def _decode_one(buf):
        self = IMUSensor()
        self.timestamp = struct.unpack(">q", buf.read(8))[0]
        self.acceleration = struct.unpack('>3f', buf.read(12))
        self.angular_velocity = struct.unpack('>3f', buf.read(12))
        return self
    _decode_one = staticmethod(_decode_one)

    def _get_hash_recursive(parents):
        if IMUSensor in parents: return 0
        tmphash = (0x2b1e734f2aee4cdf) & 0xffffffffffffffff
        tmphash  = (((tmphash<<1)&0xffffffffffffffff) + (tmphash>>63)) & 0xffffffffffffffff
        return tmphash
    _get_hash_recursive = staticmethod(_get_hash_recursive)
    _packed_fingerprint = None

    def _get_packed_fingerprint():
        if IMUSensor._packed_fingerprint is None:
            IMUSensor._packed_fingerprint = struct.pack(">Q", IMUSensor._get_hash_recursive([]))
        return IMUSensor._packed_fingerprint
    _get_packed_fingerprint = staticmethod(_get_packed_fingerprint)

    def get_hash(self):
        """Get the LCM hash of the struct"""
        return struct.unpack(">Q", IMUSensor._get_packed_fingerprint())[0]

