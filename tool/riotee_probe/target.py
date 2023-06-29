from pyocd.flash.file_programmer import FileProgrammer
from pathlib import Path
import numpy as np
import struct
from typing import Sequence
from typing import Union
from typing import Callable

from riotee_probe.protocol import *
from riotee_probe.session import RioteeProbeSession
from riotee_probe.intelhex import IntelHex16bitReader


class Target(object):
    def __init__(self, session: RioteeProbeSession):
        self._session = session

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        pass

    def halt(self):
        raise NotImplementedError

    def reset(self):
        raise NotImplementedError

    def resume(self):
        raise NotImplementedError

    def write(self, addr, data):
        raise NotImplementedError

    def read(self, addr, n: int):
        raise NotImplementedError

    def program(self, fw_path: Path, progress: Callable = None):
        raise NotImplementedError


class TargetMSP430(Target):
    def __enter__(self):
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_CONNECT)
        return self

    def __exit__(self, *exc):
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_DISCONNECT)

    def reset(self):
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_RESET)

    def resume(self):
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_RESUME)

    def halt(self):
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_HALT)

    def write(self, addr: int, data: Union[Sequence[np.uint16], np.uint16]):
        if hasattr(data, "__len__"):
            pkt = struct.pack(f"=IB{len(data)}H", addr, len(data), *data)
        else:
            pkt = struct.pack(f"=IBH", addr, 1, data)

        # One Byte is required for request type
        if len(pkt) >= DAP_VENDOR_MAX_PKT_SIZE:
            raise ValueError("Data length exceeds maximum packet size")

        rsp = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_WRITE, pkt)

    def read(self, addr, n_words: int = 1):
        pkt = struct.pack(f"=IB", addr, n_words)
        rsp = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_READ, pkt)
        rsp_arr = np.frombuffer(rsp, dtype=np.uint16)
        if n_words == 1:
            return rsp_arr[0]
        else:
            return rsp_arr

    def program(self, fw_path: Path, progress: Callable = None, verify: bool = True):
        ih = IntelHex16bitReader()
        ih.loadhex(fw_path)

        self.halt()
        # Overhead: 1B request, 4B address, 1B len -> 6B
        pkts = list(ih.iter_packets((DAP_VENDOR_MAX_PKT_SIZE - 6) // 2))

        for i, pkt in enumerate(pkts):
            self.write(pkt.address, pkt.values)
            if verify:
                rb = self.read(pkt.address, len(pkt))
                if (rb != pkt.values).any():
                    raise Exception(f"Verification failed at 0x{pkt.address:08X}!")
            if progress:
                progress((i + 1) / len(pkts))

        self.resume()


class TargetNRF52(Target):
    def __enter__(self):
        self._session._board.init()
        self._session._inited = True
        return self

    def __exit__(self, *exc):
        try:
            self._session._board.uninit()
        except Exception as e:
            print("Error during board uninit:", e)
        self._session._inited = False

    def program(self, fw_path: Path, progress: Callable = None):
        if progress is None:

            def progress(arg):
                pass

        FileProgrammer(self._session, progress=progress).program(fw_path)

    def halt(self):
        self._session.board.target.halt()

    def reset(self):
        self._session.board.target.reset()

    def resume(self):
        self._session.board.target.resume()

    def write(self, addr, data: Union[Sequence[np.uint32], np.uint32]):
        if hasattr(data, "__len__"):
            self._session.board.target.write_memory_block32(addr, data)
        else:
            self._session.board.target.write_memory(addr, data)

    def read(self, addr, n_words: int = 1):
        if n_words == 1:
            return self._session.board.target.read_memory(addr)
        else:
            return self._session.board.target.read_memory_block32(addr, n_words)
