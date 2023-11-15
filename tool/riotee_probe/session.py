from typing import Optional

from typing_extensions import Self

from pyocd.core.helpers import ConnectHelper
from pyocd.core.session import Session
from .protocol import DapRetCode
from .protocol import ID_DAP_VENDOR0


class RioteeProbeSession(Session):
    def __init__(self) -> None:
        self.product_name = None

    def __enter__(self) -> Self:
        probe = ConnectHelper.choose_probe()
        super().__init__(probe, target_override="nrf52")
        self.open(init_board=False)
        self.product_name = probe.product_name
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def vendor_cmd(self, cmd_id: int, data: Optional[bytes] = None) -> bytes:
        # Subtract command offset here, will be added again later
        cmd_id = cmd_id - ID_DAP_VENDOR0
        rsp = self.probe._link.vendor(cmd_id, data)
        if rsp[0] != DapRetCode.DAP_OK:
            raise Exception(f"Probe returned error code {rsp[0]}")
        return bytes(rsp[1:])
