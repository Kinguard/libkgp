#include "TestSysInfo.h"

#include "SysInfo.h"

#include <iostream>

using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION ( TestSysInfo );

void TestSysInfo::setUp()
{
#if 1
	cout << "\n"
	<< "Numcpus          : " << OPI::sysinfo.NumCpus() << endl
	<< "Storagedevicepath: " << OPI::sysinfo.StorageDevicePath() << endl
	<< "Storagedevice    : " << OPI::sysinfo.StorageDevice() << endl
	<< "StorageBlockdev  : " << OPI::sysinfo.StorageDeviceBlock() << endl
	<< "Storagepartition : " << OPI::sysinfo.StorageDevicePartition() << endl
	<< "NetworkDevice    : " << OPI::sysinfo.NetworkDevice() << endl
	<< "Password device  : " << OPI::sysinfo.PasswordDevice() << endl;
#endif
}

void TestSysInfo::tearDown()
{
}

void TestSysInfo::Test()
{

	switch( OPI::sysinfo.Type() )
	{
	case OPI::SysInfo::TypeArmada:
		CPPUNIT_ASSERT( OPI::SysInfo::isArmada() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOpi() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isXu4() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOlimexA20() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isPC() );
		break;
	case OPI::SysInfo::TypeOlimexA20:
		CPPUNIT_ASSERT( ! OPI::SysInfo::isArmada() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOpi() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isXu4() );
		CPPUNIT_ASSERT( OPI::SysInfo::isOlimexA20() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isPC() );
		break;
	case OPI::SysInfo::TypeOpi:
		CPPUNIT_ASSERT( ! OPI::SysInfo::isArmada() );
		CPPUNIT_ASSERT( OPI::SysInfo::isOpi() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isXu4() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOlimexA20() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isPC() );
		break;
	case OPI::SysInfo::TypePC:
		CPPUNIT_ASSERT( ! OPI::SysInfo::isArmada() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOpi() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isXu4() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOlimexA20() );
		CPPUNIT_ASSERT( OPI::SysInfo::isPC() );
		break;
	case OPI::SysInfo::TypeXu4:
		CPPUNIT_ASSERT( ! OPI::SysInfo::isArmada() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOpi() );
		CPPUNIT_ASSERT( OPI::SysInfo::isXu4() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isOlimexA20() );
		CPPUNIT_ASSERT( ! OPI::SysInfo::isPC() );
		break;
	default:
		CPPUNIT_FAIL("Unknown system type");
	}



}

void TestSysInfo::TestNetworkDevice()
{
	switch( OPI::sysinfo.Type() )
	{
	case OPI::SysInfo::TypeArmada:
		CPPUNIT_ASSERT_EQUAL( OPI::sysinfo.NetworkDevice(), string("eth0") );
		break;
	case OPI::SysInfo::TypeOlimexA20:
		CPPUNIT_ASSERT_EQUAL( OPI::sysinfo.NetworkDevice(), string("eth0") );
		break;
	case OPI::SysInfo::TypeOpi:
		CPPUNIT_ASSERT_EQUAL( OPI::sysinfo.NetworkDevice(), string("eth0") );
		break;
	case OPI::SysInfo::TypePC:
		CPPUNIT_ASSERT_EQUAL( OPI::sysinfo.NetworkDevice(), string("eth0") );
		break;
	case OPI::SysInfo::TypeXu4:
		CPPUNIT_ASSERT_EQUAL( OPI::sysinfo.NetworkDevice(), string("eth0") );
		break;
	default:
		CPPUNIT_FAIL("Missing network device for unknown type");
	}
}
