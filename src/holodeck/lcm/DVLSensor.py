"""LCM type definitions
This file automatically generated by lcm.
DO NOT MODIFY BY HAND!!!!
"""

try:
    import cStringIO.StringIO as BytesIO
except ImportError:
    from io import BytesIO
import struct

class DVLSensor(object):
    __slots__ = ["timestamp", "velocity"]

    __typenames__ = ["int64_t", "float"]

    __dimensions__ = [None, [2]]

    def __init__(self):
        self.timestamp = 0
        self.velocity = [ 0.0 for dim0 in range(2) ]

    def encode(self):
        buf = BytesIO()
        buf.write(DVLSensor._get_packed_fingerprint())
        self._encode_one(buf)
        return buf.getvalue()

    def _encode_one(self, buf):
        buf.write(struct.pack(">q", self.timestamp))
        buf.write(struct.pack('>2f', *self.velocity[:2]))

    def decode(data):
        if hasattr(data, 'read'):
            buf = data
        else:
            buf = BytesIO(data)
        if buf.read(8) != DVLSensor._get_packed_fingerprint():
            raise ValueError("Decode error")
        return DVLSensor._decode_one(buf)
    decode = staticmethod(decode)

    def _decode_one(buf):
        self = DVLSensor()
        self.timestamp = struct.unpack(">q", buf.read(8))[0]
        self.velocity = struct.unpack('>2f', buf.read(8))
        return self
    _decode_one = staticmethod(_decode_one)

    _hash = None
    def _get_hash_recursive(parents):
        if DVLSensor in parents: return 0
        tmphash = (0x13111cc3baf33cad) & 0xffffffffffffffff
        tmphash  = (((tmphash<<1)&0xffffffffffffffff) + (tmphash>>63)) & 0xffffffffffffffff
        return tmphash
    _get_hash_recursive = staticmethod(_get_hash_recursive)
    _packed_fingerprint = None

    def _get_packed_fingerprint():
        if DVLSensor._packed_fingerprint is None:
            DVLSensor._packed_fingerprint = struct.pack(">Q", DVLSensor._get_hash_recursive([]))
        return DVLSensor._packed_fingerprint
    _get_packed_fingerprint = staticmethod(_get_packed_fingerprint)

