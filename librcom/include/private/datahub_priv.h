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
#ifndef _RCOM_DATAHUB_PRIV_H_
#define _RCOM_DATAHUB_PRIV_H_

#include "addr.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

datahub_t* new_datahub(datahub_onbroadcast_t onbroadcast,
                       datahub_ondata_t ondata,
                       void *userdata);
        
void delete_datahub(datahub_t* hub);

int datahub_add_link(datahub_t* hub, addr_t *addr);
int datahub_remove_link(datahub_t* hub, addr_t *addr);

addr_t *datahub_addr(datahub_t* hub);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATAHUB_PRIV_H__

