"""LCM type definitions
This file automatically generated by lcm.
DO NOT MODIFY BY HAND!!!!
"""

try:
    import cStringIO.StringIO as BytesIO
except ImportError:
    from io import BytesIO
import struct

class RangeFinderSensor(object):
    __slots__ = ["timestamp", "count", "distances", "angles"]

    __typenames__ = ["int64_t", "int32_t", "float", "float"]

    __dimensions__ = [None, None, ["count"], ["count"]]

    def __init__(self):
        self.timestamp = 0
        self.count = 0
        self.distances = []
        self.angles = []

    def encode(self):
        buf = BytesIO()
        buf.write(RangeFinderSensor._get_packed_fingerprint())
        self._encode_one(buf)
        return buf.getvalue()

    def _encode_one(self, buf):
        buf.write(struct.pack(">qi", self.timestamp, self.count))
        buf.write(struct.pack('>%df' % self.count, *self.distances[:self.count]))
        buf.write(struct.pack('>%df' % self.count, *self.angles[:self.count]))

    def decode(data):
        if hasattr(data, 'read'):
            buf = data
        else:
            buf = BytesIO(data)
        if buf.read(8) != RangeFinderSensor._get_packed_fingerprint():
            raise ValueError("Decode error")
        return RangeFinderSensor._decode_one(buf)
    decode = staticmethod(decode)

    def _decode_one(buf):
        self = RangeFinderSensor()
        self.timestamp, self.count = struct.unpack(">qi", buf.read(12))
        self.distances = struct.unpack('>%df' % self.count, buf.read(self.count * 4))
        self.angles = struct.unpack('>%df' % self.count, buf.read(self.count * 4))
        return self
    _decode_one = staticmethod(_decode_one)

    _hash = None
    def _get_hash_recursive(parents):
        if RangeFinderSensor in parents: return 0
        tmphash = (0x6fe5dbaa3d529ab8) & 0xffffffffffffffff
        tmphash  = (((tmphash<<1)&0xffffffffffffffff) + (tmphash>>63)) & 0xffffffffffffffff
        return tmphash
    _get_hash_recursive = staticmethod(_get_hash_recursive)
    _packed_fingerprint = None

    def _get_packed_fingerprint():
        if RangeFinderSensor._packed_fingerprint is None:
            RangeFinderSensor._packed_fingerprint = struct.pack(">Q", RangeFinderSensor._get_hash_recursive([]))
        return RangeFinderSensor._packed_fingerprint
    _get_packed_fingerprint = staticmethod(_get_packed_fingerprint)

