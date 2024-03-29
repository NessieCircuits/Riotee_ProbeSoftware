from enum import IntEnum


class DapRetCode(IntEnum):
    DAP_OK = 0
    DAP_ERROR = 0xFF


ID_DAP_VENDOR0: int = 0x80

DAP_VENDOR_MAX_PKT_SIZE: int = 64


class ReqType(IntEnum):
    ID_DAP_VENDOR_VERSION = 0x80
    ID_DAP_VENDOR_POWER = 0x81
    ID_DAP_VENDOR_SBW_CONNECT = 0x82
    ID_DAP_VENDOR_SBW_DISCONNECT = 0x83
    ID_DAP_VENDOR_SBW_RESET = 0x84
    ID_DAP_VENDOR_SBW_HALT = 0x85
    ID_DAP_VENDOR_SBW_RESUME = 0x86
    ID_DAP_VENDOR_SBW_READ = 0x87
    ID_DAP_VENDOR_SBW_WRITE = 0x88
    ID_DAP_VENDOR_GPIO_SET = 0x89
    ID_DAP_VENDOR_GPIO_GET = 0x8A
    ID_DAP_VENDOR_BYPASS = 0x8B


class BypassState(IntEnum):
    BYPASS_OFF = 0
    BYPASS_ON = 1


class TargetPowerState(IntEnum):
    TARGET_POWER_OFF = 0
    TARGET_POWER_ON = 1


class IOSetState(IntEnum):
    IOSET_OUT_LOW = 0
    IOSET_OUT_HIGH = 1
    IOSET_IN = 2


class IOGetState(IntEnum):
    IOGET_LOW = 0
    IOGET_HIGH = 1
