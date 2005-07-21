/* classes: h_files */

#ifndef RXSIMPH
#define RXSIMPH
/*	Copyright (C) 1995, 1996 Tom Lord
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330, 
 * Boston, MA 02110-1301, USA. 
 */



#include "rxcset.h"
#include "rxnode.h"


#ifdef __STDC__
extern int rx_simple_rexp (struct rexp_rxnode ** answer,
			   int cset_size,
			   struct rexp_rxnode *rxnode,
			   struct rexp_rxnode ** subexps);

#else /* STDC */
extern int rx_simple_rexp ();

#endif /* STDC */





#endif  /* RXSIMPH */
