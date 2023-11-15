from typing import Generator, Tuple, Any

import numpy as np
from intelhex import IntelHex


class PacketOverflowError(Exception):
    pass


class Packet16bit:
    def __init__(self, address: int, max_size: int) -> None:
        self._max_size = max_size
        self._buffer = []
        self.address = address

    def append(self, value: int) -> None:
        if len(self._buffer) < self._max_size:
            self._buffer.append(value)
        else:
            raise PacketOverflowError("Packet buffer full")

    def __len__(self) -> int:
        return len(self._buffer)

    @property
    def values(self) -> np.ndarray:
        return np.array(self._buffer, dtype=np.uint16)


class IntelHex16bitReader(IntelHex):
    def __init__(self, source: Any = None) -> None:
        super().__init__(source)

    def iter_words(self) -> Generator[Tuple[Any, int], None, None]:
        """Iterates segments and yields addresses and corresponding 16-bit values."""
        for addr, addr_stop in self.segments():
            while addr < addr_stop:
                value = self[addr] + (self[addr + 1] << 8)
                yield addr, value
                addr += 2

    def iter_packets(self, pkt_max_size) -> Generator[Packet16bit, None, None]:
        """Iterates segments and yields packets of continuous data with specified maximum size."""
        for addr, addr_stop in self.segments():
            pkt = Packet16bit(addr, pkt_max_size)
            while addr < addr_stop:
                value = self[addr] + (self[addr + 1] << 8)
                try:
                    pkt.append(value)
                except PacketOverflowError:
                    yield pkt
                    pkt = Packet16bit(addr, pkt_max_size)
                    pkt.append(value)
                addr += 2
            yield pkt
