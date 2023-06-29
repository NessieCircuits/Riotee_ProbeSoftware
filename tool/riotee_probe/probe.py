from enum import Enum
import struct
from contextlib import contextmanager

from riotee_probe.protocol import ReqType
from riotee_probe.protocol import IOSetState
from riotee_probe.session import RioteeProbeSession
from riotee_probe.target import TargetNRF52
from riotee_probe.target import TargetMSP430


class GpioDir(Enum):
    GPIO_DIR_IN = 0
    GPIO_DIR_OUT = 1


@contextmanager
def get_connected_probe():
    with RioteeProbeSession() as session:
        if session.product_name == "Riotee Board":
            yield RioteeProbeBoard(session)
        elif session.product_name == "Riotee Probe":
            yield RioteeProbeProbe(session)
        else:
            raise Exception(f"Unsupported probe {session.product_name} selected")


class RioteeProbe(object):
    def __init__(self, session):
        self._session = session

    @contextmanager
    def msp430(self):
        with TargetMSP430(self._session) as msp430:
            yield msp430

    @contextmanager
    def nrf52(self):
        with TargetNRF52(self._session) as nrf52:
            yield nrf52

    def target_power(self, state: bool):
        pkt = struct.pack("=B", state)
        return self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_POWER, pkt)

    def gpio_set(self, pin: int, state: bool):
        raise NotImplementedError

    def gpio_get(self, pin: int):
        raise NotImplementedError

    def bypass(self, state):
        raise NotImplementedError

    def fw_version(self) -> str:
        ret = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_VERSION)
        return str(ret, encoding="utf-8")


class RioteeProbeProbe(RioteeProbe):
    def gpio_set(self, pin: int, state: bool):
        if state:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_HIGH)
        else:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_LOW)

        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_SET, pkt)

    def gpio_get(self, pin: int) -> bool:
        pkt = struct.pack("=B", pin)

        rsp = self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_GET, pkt)
        return bool(rsp[0])

    def gpio_dir(self, pin: int, dir: GpioDir):
        if dir == GpioDir.GPIO_DIR_IN:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_IN)
        else:
            pkt = struct.pack("=BB", pin, IOSetState.IOSET_OUT_LOW)

        self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_GPIO_SET, pkt)


class RioteeProbeBoard(RioteeProbe):
    def bypass(self, state: bool):
        pkt = struct.pack("=B", state)
        return self._session.vendor_cmd(ReqType.ID_DAP_VENDOR_BYPASS, pkt)
