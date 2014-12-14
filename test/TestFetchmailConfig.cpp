#include "FetchmailConfig.h"
#include "TestFetchmailConfig.h"

#include <unistd.h>
#include <fstream>

using namespace std;
using namespace OPI;

CPPUNIT_TEST_SUITE_REGISTRATION ( TestFetchmailConfig );

void TestFetchmailConfig::setUp()
{
	ofstream out("test.fil");

	out << "poll poll poll\n"
	<< "set postmaster \"postmaster\"\n"
	<< "set bouncemail\n"
	<< "set no spambounce\n"
	<< "set properties \"\"\n"
	<< "set daemon 15\n"
	<< "set syslog\n"
	<< "\n"
	<< "poll    pop3.mymailhost.nu with proto POP3 \n"
	<< "	interval 5\n"
	<< "	user 'tor@krill.nu' there with password 'd32 2d2d' is 'bengt' here ssl smtpaddress localdomain\n"
	<< "	user 'info@krill.nu' there with password 'd2d243r2r' is 'sven' here ssl\n"
	<< "\n"
	<< "#poll    pop3.mymailhost.nu with proto POP3 \n"
	<< "#	interval 5\n"
	<< " #	user 'tor@krill.nu' there with password 'd32 2d2d' is 'bengt' here ssl smtpaddress localdomain\n"
	<< "#	user 'info@krill.nu' there with password 'd2d243r2r' is 'sven' here ssl\n"
	<< "\n"
	<< "poll  pop3.gmail.se with proto POP3 \n"
	<< "	interval 5\n"
	<< "	user 'kalle' there with password 'd3 22d2d' is 'sven' here smtpaddress localdomain\n"
	<< "	user 'bengt' there with password 'd2d243r2r' is 'bengt' here ssl smtpaddress localdomain\n";

	out.close();

}

void TestFetchmailConfig::tearDown()
{
	unlink("test.fil");
	unlink("test.fil.lnk");
}

void TestFetchmailConfig::TestBasicUsage()
{
	FetchmailConfig fc("test.fil");

	CPPUNIT_ASSERT_THROW( fc.GetAccount("Nohost","noid"), runtime_error);
	CPPUNIT_ASSERT_THROW( fc.GetAccount("Nohost",""), runtime_error);
	CPPUNIT_ASSERT_THROW( fc.GetAccount("","noid"), runtime_error);
	CPPUNIT_ASSERT_THROW( fc.GetAccount("",""), runtime_error);

	CPPUNIT_ASSERT_NO_THROW( fc.GetAccount("pop3.mymailhost.nu","tor@krill.nu") );
	CPPUNIT_ASSERT_NO_THROW( fc.GetAccount("pop3.mymailhost.nu","info@krill.nu") );
	CPPUNIT_ASSERT_THROW( fc.GetAccount("pop3.mymailhost.nu",""), runtime_error);

	CPPUNIT_ASSERT_EQUAL( (size_t)2, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)2, fc.GetAccounts("bengt").size() );

	list<map<string,string>> accounts;
	CPPUNIT_ASSERT_NO_THROW( accounts = fc.GetAccounts() );

	CPPUNIT_ASSERT_EQUAL( (size_t) 4, accounts.size() );

	// Write and read back
	fc.WriteConfig();

	fc.ReadConfig();
	CPPUNIT_ASSERT_EQUAL( (size_t)2, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)2, fc.GetAccounts("bengt").size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)4, fc.GetAccounts().size() );

	CPPUNIT_ASSERT_NO_THROW( fc.AddAccount("me@myself.me", "new.host.me","my identity","Mys3cr$t P+$$w0rd","user",false));
	CPPUNIT_ASSERT_THROW( fc.AddAccount("me@myself.me","new.host.me","my identity","Mys3cr$t P+$$w0rd","user",false), runtime_error);
	CPPUNIT_ASSERT_EQUAL( (size_t)3, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)5, fc.GetAccounts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)1, fc.GetAccounts("user").size() );

	fc.WriteConfig();
	fc.ReadConfig();

	map<string,string> user;
	CPPUNIT_ASSERT_NO_THROW( user = fc.GetAccount("new.host.me","my identity") );
	CPPUNIT_ASSERT_EQUAL( string("me@myself.me"),		user["email"]);
	CPPUNIT_ASSERT_EQUAL( string("new.host.me"),		user["host"]);
	CPPUNIT_ASSERT_EQUAL( string("my identity"),		user["identity"]);
	CPPUNIT_ASSERT_EQUAL( string("Mys3cr$t P+$$w0rd"),	user["password"]);
	CPPUNIT_ASSERT_EQUAL( string("user"),				user["username"]);
	CPPUNIT_ASSERT_EQUAL( string("false"),				user["ssl"]);

	CPPUNIT_ASSERT_NO_THROW( fc.UpdateAccount("me@myself.me","new.host.me","my identity","nosecret","newuser",false) );

	fc.WriteConfig();
	fc.ReadConfig();

	CPPUNIT_ASSERT_NO_THROW( user = fc.GetAccount("new.host.me","my identity") );
	CPPUNIT_ASSERT_EQUAL( string("me@myself.me"),		user["email"]);
	CPPUNIT_ASSERT_EQUAL( string("new.host.me"),	user["host"]);
	CPPUNIT_ASSERT_EQUAL( string("my identity"),	user["identity"]);
	CPPUNIT_ASSERT_EQUAL( string("nosecret"),		user["password"]);
	CPPUNIT_ASSERT_EQUAL( string("newuser"),		user["username"]);
	CPPUNIT_ASSERT_EQUAL( string("false"),				user["ssl"]);

	CPPUNIT_ASSERT_NO_THROW( fc.DeleteAccount("pop3.mymailhost.nu","tor@krill.nu") );
	CPPUNIT_ASSERT_THROW( fc.GetAccount("pop3.mymailhost.nu","tor@krill.nu"), runtime_error );
	CPPUNIT_ASSERT_EQUAL( (size_t)3, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)4, fc.GetAccounts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)1, fc.GetAccounts("bengt").size() );

	CPPUNIT_ASSERT_NO_THROW( fc.DeleteAccount("pop3.mymailhost.nu","info@krill.nu") );
	CPPUNIT_ASSERT_EQUAL( (size_t)2, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)3, fc.GetAccounts().size() );

	CPPUNIT_ASSERT_NO_THROW( fc.DeleteAccount("new.host.me","my identity") );
	CPPUNIT_ASSERT_NO_THROW( fc.DeleteAccount("pop3.gmail.se","kalle") );
	CPPUNIT_ASSERT_NO_THROW( fc.DeleteAccount("pop3.gmail.se","bengt") );

	CPPUNIT_ASSERT_EQUAL( (size_t)0, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)0, fc.GetAccounts().size() );

	CPPUNIT_ASSERT_NO_THROW(fc.WriteConfig() );
	CPPUNIT_ASSERT_NO_THROW( fc.ReadConfig() );

	CPPUNIT_ASSERT_EQUAL( (size_t)0, fc.GetHosts().size() );
	CPPUNIT_ASSERT_EQUAL( (size_t)0, fc.GetAccounts().size() );
}

void TestFetchmailConfig::TestSSL()
{
	FetchmailConfig fc("test.fil");

	CPPUNIT_ASSERT_NO_THROW( fc.AddAccount("u1@krill.nu", "pop4.mymailhost.nu","Test ID1","Mys3cr$t P+$$w0rd","user"));
	CPPUNIT_ASSERT_NO_THROW( fc.AddAccount("u2@krill.nu", "pop4.mymailhost.nu","Test ID2","Mys3cr$t P+$$w0rd","user", true));

	fc.WriteConfig();
	fc.ReadConfig();

	map<string,string> user;
	CPPUNIT_ASSERT_NO_THROW( user = fc.GetAccount("pop4.mymailhost.nu","Test ID1") );
	CPPUNIT_ASSERT_EQUAL( string("false"), user["ssl"] );

	CPPUNIT_ASSERT_NO_THROW( user = fc.GetAccount("pop4.mymailhost.nu","Test ID2") );
	CPPUNIT_ASSERT_EQUAL( string("true"), user["ssl"] );
}

void TestFetchmailConfig::TestErrorCases()
{
	FetchmailConfig fc("no.fil");
	CPPUNIT_ASSERT_NO_THROW( fc.ReadConfig()	);

	CPPUNIT_ASSERT_NO_THROW( fc.GetAccounts() );
}
