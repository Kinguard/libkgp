#ifndef TESTDISKHELPER_H_
#define TESTDISKHELPER_H_

#include <cppunit/extensions/HelperMacros.h>

class TestDiskHelper: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( TestDiskHelper );
	CPPUNIT_TEST( Test );
	CPPUNIT_TEST( TestIsMounted );
	CPPUNIT_TEST( TestStorageDevices );
	CPPUNIT_TEST( TestPartitionName );
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void tearDown();
	void Test();
	void TestIsMounted();
	void TestStorageDevices();
	void TestPartitionName();
};

#endif /* TESTDISKHELPER_H_ */
