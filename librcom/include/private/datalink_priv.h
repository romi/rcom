/*
  rcom

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcom is light-weight libary for inter-node communication.

  rcom is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */
#ifndef _RCOM_DATALINK_PRIV_H_
#define _RCOM_DATALINK_PRIV_H_

#include "datalink.h"
#include "addr.h"

#ifdef __cplusplus
extern "C" {
#endif

datalink_t *new_datalink(datalink_ondata_t callback, void* userdata);
void delete_datalink(datalink_t *datalink);
addr_t *datalink_addr(datalink_t *datalink);
addr_t *datalink_remote_addr(datalink_t *datalink);
void datalink_set_remote_addr(datalink_t *datalink, addr_t *addr);

int datalink_sendto(datalink_t *datalink, addr_t *addr, data_t *data);


#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATALINK_PRIV_H_

