import struct
from contextlib import contextmanager
from enum import Enum
from typing import Generator

from .protocol import IOSetState, ReqType
from .session import RioteeProbeSession
from .target import TargetMSP430, TargetNRF52


class GpioDir(Enum):
    GPIO_DIR_IN = 0
    GPIO_DIR_OUT = 1


class RioteeProbe:
    def __init__(self, session: RioteeProbeSession) -> None:
        self._session = session

    @contextmanager
    def msp430(self) -> Generator[TargetMSP430, None, None]:
        with TargetMSP430(self._session) as msp430:
            yield msp430

    @contextmanager
    def nrf52(self) -> Generator[TargetNRF52, None, None]:
        with TargetNRF52(self._session) as nrf52:
            yield nrf52

    def target_power(self, state: bool):
        pkt = struct.pack("=B", state)
        return self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_POWER, pkt)

    def gpio_dir(self, pin: int, dir: GpioDir) -> None:
        raise NotImplementedError

    def gpio_set(self, pin: int, state: bool) -> None:
        raise NotImplementedError

    def gpio_get(self, pin: int) -> bool:
        raise NotImplementedError

    def bypass(self, state: bool) -> None:
        raise NotImplementedError

    def fw_version(self) -> str:
        ret = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_VERSION)
        # Firmware versions before 1.1.0 send a trailing nul over the wire
        return str(ret.strip(b"\0"), encoding="utf-8")


@contextmanager
def get_connected_probe() -> Generator[RioteeProbe, None, None]:
    with RioteeProbeSession() as session:
        if session.product_name == "Riotee Board":
            yield RioteeProbeBoard(session)
        elif session.product_name == "Riotee Probe":
            yield RioteeProbeProbe(session)
        else:
            raise Exception(f"Unsupported probe {session.product_name} selected")


class RioteeProbeProbe(RioteeProbe):
    def gpio_set(self, pin: int, state: bool) -> None:
        if state:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_HIGH)
        else:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_LOW)

        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_SET, pkt)

    def gpio_get(self, pin: int) -> bool:
        pkt = struct.pack("=B", pin)

        rsp = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_GET, pkt)
        return bool(rsp[0])

    def gpio_dir(self, pin: int, dir: GpioDir) -> None:
        if dir == GpioDir.GPIO_DIR_IN:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_IN)
        else:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_LOW)

        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_SET, pkt)


class RioteeProbeBoard(RioteeProbe):
    def bypass(self, state: bool):
        pkt = struct.pack("=B", state)
        return self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_BYPASS, pkt)
