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
#ifndef _RCOM_APP_H_
#define _RCOM_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

void app_init(int *argc, char **argv);

int app_quit();
void app_set_quit();

void app_set_name(const char *path);
const char *app_get_name();
const char *app_ip();

void app_set_logdir(const char *logdir);
const char *app_get_logdir();

void app_set_session(const char *session);
const char *app_get_session();

const char *app_get_config();

int app_print();
int app_standalone();

#ifdef __cplusplus
}
#endif

#endif // _RCOM_APP_H_
