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
#ifndef _RCOM_STREAMERLINK_PRIV_H_
#define _RCOM_STREAMERLINK_PRIV_H_

#include "streamerlink.h"

#ifdef __cplusplus
extern "C" {
#endif

streamerlink_t *new_streamerlink(streamerlink_ondata_t ondata,
                                 streamerlink_onresponse_t onresponse,
                                 void* data, int autoconnect);
        
void delete_streamerlink(streamerlink_t *link);

int streamerlink_set_remote(streamerlink_t *link, addr_t *addr);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMERLINK_PRIV_H_
