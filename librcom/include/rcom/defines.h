/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#ifndef _RCOM_DEFINES_H_
#define _RCOM_DEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif
        
#define RCOM_WAIT_ERROR  -1
#define RCOM_WAIT_OK      1
#define RCOM_WAIT_TIMEOUT 0 

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DEFINES_H_
