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
#ifndef _LIBRCOM_I_RESPONSE_PARSER_H_
#define _LIBRCOM_I_RESPONSE_PARSER_H_

#include "IResponse.h"
#include "ISocket.h"

namespace rcom {

        class IResponseParser
        {
        protected:

        public:
                
                virtual ~IResponseParser() = default; 

                virtual bool parse(ISocket& socket) = 0;
                virtual IResponse& response() = 0;
        };
}

#endif // _LIBRCOM_I_RESPONSE_PARSER_H_

