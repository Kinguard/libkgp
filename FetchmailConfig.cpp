
#include <sstream>

#include "FetchmailConfig.h"

#include <libutils/FileUtils.h>
#include <libutils/String.h>

using namespace Utils;

namespace OPI
{

FetchmailConfig::FetchmailConfig(const string &cfgpath):
	configfile(cfgpath),
	host("^poll\\s+(\\S+)\\s+with proto"),
	user("\\s+user\\s+(\\S+)\\s+there with password\\s+(.+)\\s+is\\s+(\\S+)\\s+here\\s+(ssl|)"),
	cfgfile(cfgpath+".lnk")
{
	this->ReadConfig();
}

void FetchmailConfig::AddAccount(const string &email, const string &host, const string &identity, const string &password, const string &user, bool use_ssl)
{
	if( this->_hasuser( host, identity ) )
	{
		throw runtime_error("User already exists");
	}
	this->cfgfile[host+"\t"+identity] = email;
	this->config[host][identity] = {password, user, use_ssl};

}

void FetchmailConfig::UpdateAccount(const string &email, const string &host, const string &identity, const string &password, const string &user, bool use_ssl)
{
	if( ! this->_hasuser( host, identity ) )
	{
		throw runtime_error("User doesnt exists");
	}

	this->cfgfile[host+"\t"+identity] = email;
	this->config[host][identity] = {password, user, use_ssl};
}

list<string> FetchmailConfig::GetHosts()
{
	list<string> hosts;
	for( const auto& host: this->config )
	{
		hosts.push_back( host.first );
	}
	return hosts;
}

map<string, string> FetchmailConfig::GetAccount(const string &host, const string &identity)
{
	if( ! this->_hasuser( host, identity ) )
	{
		throw runtime_error("User doesnt exists");
	}

	return {
		{"email",		this->cfgfile[host+"\t"+identity]},
		{"host",		host},
		{"identity",	identity},
		{"password",	this->config[host][identity].password},
		{"username",	this->config[host][identity].user},
		{"ssl",			this->config[host][identity].use_ssl?"true":"false"},
	};

}

list<map<string, string> > FetchmailConfig::GetAccounts(const string &user)
{
	list<map<string, string> > ret;
	bool fetchall = user == "";
	for( const auto& domain: this->config )
	{
		for( const auto& identity: domain.second )
		{
			map<string,string> userline = this->GetAccount(domain.first, identity.first);
			if( fetchall || user == userline["username"] )
			{
				ret.push_back( userline );
			}
		}
	}

	return ret;
}

void FetchmailConfig::DeleteAccount(const string &host, const string &identity)
{
	if( ! this->_hasuser( host, identity ) )
	{
		throw runtime_error("User doesnt exists");
	}

	this->config[host].erase(identity);
	this->cfgfile.erase(host+"\t"+identity);
	// Last identity at host?
	if( this->config[host].size() == 0 )
	{
		this->config.erase( host );
	}
}

static inline string getmatchstring(const string& line, Regex::Match m)
{
	return line.substr(m.rm_so, m.rm_eo - m.rm_so);
}

void FetchmailConfig::ReadConfig()
{
	this->config.empty();
	list<string> lines = File::GetContent( this->configfile );

	string lasthost="";
	for( const string& line: lines)
	{
		vector<Regex::Match> m;
		if( (m = host.DoMatch(line)).size() > 1)
		{
			lasthost = line.substr( m[1].rm_so, m[1].rm_eo-m[1].rm_so );
		}
		else if( (m = user.DoMatch(line)).size() >= 4 )
		{
			if( lasthost ==  "")
			{
				continue;
			}
			string identity =	String::Trimmed( getmatchstring( line, m[1]), "'\"" );
			string password =	String::Trimmed( getmatchstring( line, m[2]), "'\"" );
			string user =		String::Trimmed( getmatchstring( line, m[3]), "'\"" );
			bool usessl =  ( m.size() == 5) && ( getmatchstring( line, m[4]) == "ssl" );
			this->config[lasthost][identity] = { password, user, usessl };
		}
	}
}

void FetchmailConfig::WriteConfig()
{
	stringstream out;

	out << "set postmaster \"postmaster\"\n"
		<< "set bouncemail\n"
		<< "set no spambounce\n"
		<< "set properties \"\"\n"
		<< "set daemon 300\n"
		<< "set syslog";

	for( const auto& cfgline: config)
	{
		string host = cfgline.first;

		out << "\npoll "<<host<<" with proto POP3\n";

		map< string, struct userinfo > users = cfgline.second;
		for( const auto& user: users )
		{
			out << "\tuser '"<<user.first <<"' there with password '"
				<<user.second.password << "' is '"<<user.second.user
			   << "' here";
			if( user.second.use_ssl )
			{
				out << " ssl";
			}
			out	<< " smtpaddress localdomain\n";
		}
	}
	this->cfgfile.Sync(true, 0600);
	File::Write( this->configfile, out.str(), 0600);
}

FetchmailConfig::~FetchmailConfig()
{

}

bool FetchmailConfig::_hasuser(const string &host, const string &identity)
{
	return ( this->config.find(host) != this->config.end() ) &&
			( this->config[host].find(identity) != this->config[host].end() );
}

} // End namespace
