#ifndef __SWD_TRANSPORT_H_

void swd_transport_read_mode(void);
void swd_transport_write_mode(void);
void swd_transport_init();

void swd_transport_connect();
void swd_transport_disconnect();

#endif /* __SWD_TRANSPORT_H_ */
