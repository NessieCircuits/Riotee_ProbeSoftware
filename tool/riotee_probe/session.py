from contextlib import contextmanager
from pyocd.core.helpers import ConnectHelper
from pyocd.core.session import Session
from riotee_probe.protocol import DapRetCode
from riotee_probe.protocol import ID_DAP_VENDOR0


class RioteeProbeSession(Session):
    def __init__(self):
        self.product_name = None

    def __enter__(self):
        probe = ConnectHelper.choose_probe()
        super().__init__(probe, target_override="nrf52")
        self.open(init_board=False)
        self.product_name = probe.product_name
        return self

    def __exit__(self, *exc):
        self.close()

    def vendor_cmd(self, cmd_id: int = None, data: bytes = None) -> bytes:
        # Subtract command offset here, will be added again later
        cmd_id = cmd_id - ID_DAP_VENDOR0
        rsp = self.probe._link.vendor(cmd_id, data)
        if rsp[0] != DapRetCode.DAP_OK:
            raise Exception(f"Probe returned error code {rsp[0]}")
        return bytes(rsp[1:])
