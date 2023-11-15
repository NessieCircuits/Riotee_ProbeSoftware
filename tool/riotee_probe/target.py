import struct
from pathlib import Path
from typing import Callable, Optional, Sequence, Union

import numpy as np
from pyocd.flash.file_programmer import FileProgrammer
from typing_extensions import Self

from .intelhex import IntelHex16bitReader
from .protocol import DAP_VENDOR_MAX_PKT_SIZE, ReqType
from .session import RioteeProbeSession


class Target:
    def __init__(self, session: RioteeProbeSession) -> None:
        self._session = session

    def __enter__(self) -> Self:
        return self

    def __exit__(self, *exc) -> None:
        pass

    def halt(self) -> None:
        raise NotImplementedError

    def reset(self) -> None:
        raise NotImplementedError

    def resume(self) -> None:
        raise NotImplementedError

    def write(self, addr, data) -> None:
        raise NotImplementedError

    def read(self, addr, n: int) -> None:
        raise NotImplementedError

    def program(self, fw_path: Path, progress: Optional[Callable] = None) -> None:
        raise NotImplementedError


class TargetMSP430(Target):
    def __enter__(self) -> Self:
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_CONNECT)
        return self

    def __exit__(self, *exc) -> None:
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_DISCONNECT)

    def reset(self) -> None:
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_RESET)

    def resume(self) -> None:
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_RESUME)

    def halt(self) -> None:
        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_HALT)

    def write(self, addr: int, data: Union[Sequence[np.uint16], np.uint16]) -> None:
        if hasattr(data, "__len__"):
            pkt = struct.pack(f"=IB{len(data)}H", addr, len(data), *data)
        else:
            pkt = struct.pack("=IBH", addr, 1, data)

        # One Byte is required for request type
        if len(pkt) >= DAP_VENDOR_MAX_PKT_SIZE:
            raise ValueError("Data length exceeds maximum packet size")

        _ = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_WRITE, pkt)

    def read(self, addr, n_words: int = 1) -> np.ndarray:
        pkt = struct.pack("=IB", addr, n_words)
        rsp = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_SBW_READ, pkt)
        rsp_arr = np.frombuffer(rsp, dtype=np.uint16)
        if n_words == 1:
            return rsp_arr[0]
        return rsp_arr

    def program(self, fw_path: Path, progress: Optional[Callable] = None, verify: bool = True) -> None:
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
    def __enter__(self) -> Self:
        self._session._board.init()
        self._session._inited = True
        return self

    def __exit__(self, *exc) -> None:
        try:
            self._session._board.uninit()
        except Exception as e:
            print("Error during board uninit:", e)
        self._session._inited = False

    def program(self, fw_path: Path, progress: Optional[Callable] = None) -> None:
        if progress is None:

            def progress(_) -> None:
                pass

        FileProgrammer(self._session, progress=progress).program(str(fw_path))

    def halt(self) -> None:
        self._session.board.target.halt()

    def reset(self) -> None:
        self._session.board.target.reset()

    def resume(self) -> None:
        self._session.board.target.resume()

    def write(self, addr, data: Union[Sequence[np.uint32], np.uint32]) -> None:
        if hasattr(data, "__len__"):
            self._session.board.target.write_memory_block32(addr, data)
        else:
            self._session.board.target.write_memory(addr, data)

    def read(self, addr, n_words: int = 1):
        if n_words == 1:
            return self._session.board.target.read_memory(addr)
        return self._session.board.target.read_memory_block32(addr, n_words)
