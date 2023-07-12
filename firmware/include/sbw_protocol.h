#ifndef __SBW_PROTOCOL_H_
#define __SBW_PROTOCOL_H_

#include <stdint.h>

enum {
  SBW_REQ_START,
  SBW_REQ_STOP,
  SBW_REQ_HALT,
  SBW_REQ_RELEASE,
  SBW_REQ_WRITE,
  SBW_REQ_READ,
  SBW_REQ_POWER,
  SBW_REQ_IOSET,
  SBW_REQ_IOGET,
  SBW_REQ_BYPASS,
};

enum { TARGET_POWER_OFF = 0, TARGET_POWER_ON = 1 };
enum { BYPASS_OFF = 0, BYPASS_ON = 1 };
enum { IOSET_OUT_LOW = 0, IOSET_OUT_HIGH = 1, IOSET_IN = 2 };
enum { IOGET_LOW = 0, IOGET_HIGH = 1 };

typedef uint8_t probe_io_state_t;

enum {
  SBW_RC_OK,
  SBW_RC_ERR_GENERIC,
  SBW_RC_ERR_UNKNOWN_REQ,
  SBW_RC_ERR_UNSUPPORTED
};

/* Format of packets received from the host */
typedef struct __attribute__((packed)) {
  uint8_t req_type;
  uint8_t len;
  uint32_t address;
  uint16_t data[29];
} sbw_req_t;

/* Format of packets sent to the host */
typedef struct __attribute__((packed)) {
  uint8_t rc;
  uint8_t len;
  uint16_t data[31];
} sbw_rsp_t;

#endif /* __SBW_PROTOCOL_H_ */