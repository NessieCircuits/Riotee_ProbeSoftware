/*
 * Copyright (c) 2023 Nessie Circuits
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SWD_TRANSPORT_H_


void swd_transport_read_mode(void);
void swd_transport_write_mode(void);
void swd_transport_init();

void swd_transport_connect();
void swd_transport_disconnect();

#endif /* __SWD_TRANSPORT_H_ */
