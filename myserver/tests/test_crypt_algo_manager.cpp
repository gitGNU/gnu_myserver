/*
 MyServer
 Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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

#include <include/base/crypt/crypt_algo_manager.h>
#include <include/base/crypt/md5.h>

#include <ctype.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string.h>

#include <typeinfo>
using namespace std;

class TestCryptAlgoManager : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestCryptAlgoManager );
  CPPUNIT_TEST ( testRegister );
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {

  }

  void tearDown ()
  {
  }

  void testRegister ()
  {
    string name ("md5");
    CryptAlgoManager cam;
    CryptAlgo *cal;

    cal = cam.make (name);
    CPPUNIT_ASSERT_EQUAL (cal, (CryptAlgo*) NULL);

    Md5::initialize (&cam);

    cal = cam.make (name);
    CPPUNIT_ASSERT (cal);
    CPPUNIT_ASSERT (typeid (*cal) == typeid (Md5));
    delete cal;

    cam.registerAlgorithm (name, NULL);
    cal = cam.make (name);
    CPPUNIT_ASSERT_EQUAL (cal, (CryptAlgo*) NULL);
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION (TestCryptAlgoManager);
