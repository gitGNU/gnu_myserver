/*
*MyServer
*Copyright (C) 2002,2003,2004 The MyServer Team
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef COUNTER_OUTPUT_H
#define COUNTER_OUTPUT_H

#include "../../../cgi-lib/cgi_manager.h"
class Counter_Output
{
    private:
    	unsigned char *outBuffer;
    	unsigned long int number;
    public:
    void setNumber(unsigned long int );
	void setWrite(CgiManager *);
	void run();
};

#endif