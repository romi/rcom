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

#include <stdexcept>
#include <r.h>
#include "util.h"
#include "App.h"

namespace rcom {
        
        App::App() : id_()
        {
                create_uuid(id_);
        }
        
        App::App(const char *id) : id_()
        {
                if (!set_id(id)) {
                        r_err("App::set failed: id=%s", id);
                        throw std::runtime_error("Address::set_id failed");
                }
        }

        bool App::set_id(const char *str)
        {
                bool success = false;
                if (is_valid_uuid(str)) {
                        id_ = str;
                        success = true;
                }
                return success;
        }
}

