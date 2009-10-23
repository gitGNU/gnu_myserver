/* -*- mode: c++ -*- */
/*
  MyServer
  Copyright (C) 2009 Free Software Foundation, Inc.
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CRYPT_ALGO_MANAGER_H
# define CRYPT_ALGO_MANAGER_H

# include "stdafx.h"

# include <include/base/hash_map/hash_map.h>
# include <include/base/crypt/crypt_algo.h>

class CryptAlgoManager
{
public:
  typedef CryptAlgo* (*builder) ();
  CryptAlgoManager ();
  ~CryptAlgoManager ();
  void registerAlgorithm (string &name, builder bld);
  CryptAlgo *make (string &s);
  bool check (string &value, string &result, string &algo);
private:
  HashMap<string, builder> algorithms;
};

#endif
