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
#ifndef _RCOM_MESSAGEHUB_PRIV_H_
#define _RCOM_MESSAGEHUB_PRIV_H_

#include "rcom/messagehub.h"
#include "rcom/addr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Set the port equal to 0 to let the OS pick one for you.
messagehub_t *new_messagehub(const char *name,
                             int port,
                             messagehub_onconnect_t onconnect,
                             void *userdata);
void delete_messagehub(messagehub_t *hub);
addr_t *messagehub_addr(messagehub_t *hub);
const char *messagehub_name(messagehub_t *hub);

void messagehub_remove_link(messagehub_t *hub, messagelink_t *link);
                
#ifdef __cplusplus
}
#endif

#endif // _RCOM_ENDPOINT_PRIV_H_
