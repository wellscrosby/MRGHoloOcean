"""LCM type definitions
This file automatically generated by lcm.
DO NOT MODIFY BY HAND!!!!
"""

try:
    import cStringIO.StringIO as BytesIO
except ImportError:
    from io import BytesIO
import struct

class PoseSensor(object):
    __slots__ = ["timestamp", "matrix"]

    __typenames__ = ["int64_t", "float"]

    __dimensions__ = [None, [4, 4]]

    def __init__(self):
        self.timestamp = 0
        self.matrix = [ [ 0.0 for dim1 in range(4) ] for dim0 in range(4) ]

    def encode(self):
        buf = BytesIO()
        buf.write(PoseSensor._get_packed_fingerprint())
        self._encode_one(buf)
        return buf.getvalue()

    def _encode_one(self, buf):
        buf.write(struct.pack(">q", self.timestamp))
        for i0 in range(4):
            buf.write(struct.pack('>4f', *self.matrix[i0][:4]))

    def decode(data):
        if hasattr(data, 'read'):
            buf = data
        else:
            buf = BytesIO(data)
        if buf.read(8) != PoseSensor._get_packed_fingerprint():
            raise ValueError("Decode error")
        return PoseSensor._decode_one(buf)
    decode = staticmethod(decode)

    def _decode_one(buf):
        self = PoseSensor()
        self.timestamp = struct.unpack(">q", buf.read(8))[0]
        self.matrix = []
        for i0 in range(4):
            self.matrix.append(struct.unpack('>4f', buf.read(16)))
        return self
    _decode_one = staticmethod(_decode_one)

    def _get_hash_recursive(parents):
        if PoseSensor in parents: return 0
        tmphash = (0x5b2182005827a64) & 0xffffffffffffffff
        tmphash  = (((tmphash<<1)&0xffffffffffffffff) + (tmphash>>63)) & 0xffffffffffffffff
        return tmphash
    _get_hash_recursive = staticmethod(_get_hash_recursive)
    _packed_fingerprint = None

    def _get_packed_fingerprint():
        if PoseSensor._packed_fingerprint is None:
            PoseSensor._packed_fingerprint = struct.pack(">Q", PoseSensor._get_hash_recursive([]))
        return PoseSensor._packed_fingerprint
    _get_packed_fingerprint = staticmethod(_get_packed_fingerprint)

    def get_hash(self):
        """Get the LCM hash of the struct"""
        return struct.unpack(">Q", PoseSensor._get_packed_fingerprint())[0]

