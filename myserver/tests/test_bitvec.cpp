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
 
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>


#include "../include/base/bitvec/bitvec.h"

class TestBitVec : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE ( TestBitVec );
  
  CPPUNIT_TEST ( testSet );
  CPPUNIT_TEST ( testFfs );
  CPPUNIT_TEST ( testGet );
  
  CPPUNIT_TEST_SUITE_END ();

public:
  void setUp ()
  {
  }
  
  void tearDown ()
  {
  }


  void testFfs ()
  {
    BitVec vec (200, false);
    CPPUNIT_ASSERT_EQUAL (vec.ffs (), -1);

    vec.set (10);

    CPPUNIT_ASSERT_EQUAL (vec.ffs (), 10);
  }


  void testSet ()
  {
    BitVec vec (200, false);
    CPPUNIT_ASSERT_EQUAL (vec.get (0), false);
    vec.set (0);
    CPPUNIT_ASSERT_EQUAL (vec.get (0), true);
    vec.unset (0);
    CPPUNIT_ASSERT_EQUAL (vec.get (0), false);
  }
  

  void testGet ()
  {
    BitVec vecFalse (200, false);

    CPPUNIT_ASSERT_EQUAL (vecFalse.get (0), false);
    CPPUNIT_ASSERT_EQUAL (vecFalse.get (10), false);
    CPPUNIT_ASSERT_EQUAL (vecFalse.get (100), false);

    BitVec vecTrue (200, true);

    CPPUNIT_ASSERT_EQUAL (vecFalse.get (0), true);
    CPPUNIT_ASSERT_EQUAL (vecFalse.get (10), true);
    CPPUNIT_ASSERT_EQUAL (vecFalse.get (100), true);
  }


};

CPPUNIT_TEST_SUITE_REGISTRATION (TestBitVec);
