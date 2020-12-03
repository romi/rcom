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
#ifndef _RCOM_STREAMER_PRIV_H_
#define _RCOM_STREAMER_PRIV_H_

#include "streamer.h"

// Pass a port equal to 0 to open the streamer on any available port.
streamer_t *new_streamer(const char *name,
                         const char *topic,
                         int port,
                         const char *mimetype,
                         streamer_onclient_t onclient,
                         streamer_onbroadcast_t onbroadcast,
                         void *userdata);

void delete_streamer(streamer_t *streamer);

#endif // _RCOM_STREAMER_PRIV_H_
