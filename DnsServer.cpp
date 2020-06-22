#include "DnsServer.h"
#include "CryptoHelper.h"
#include "Config.h"
#include "SysInfo.h"
#include "SysConfig.h"
#include "NetworkConfig.h"

#include <libutils/Logger.h>
#include <libutils/FileUtils.h>

#include <iterator>
#include <algorithm>

using namespace Utils;

namespace OPI
{

using namespace CryptoHelper;

DnsServer::DnsServer(const string &host): HttpClient(host)
{
}

tuple<int, Json::Value> DnsServer::CheckOPIName(const string &opiname)
{
	map<string,string> postargs = {
        {"fqdn", opiname},
		{"checkname",  "1"}
	};

	string body = this->DoPost("update_dns.php", postargs);

	Json::Value retobj = Json::objectValue;
	this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

bool DnsServer::RegisterPublicKey(const string &unit_id, const string &key, const string &token)
{
	logg << Logger::Debug << "Register dns public key "<< lend;

	map<string,string> postargs = {
		{"unit_id", unit_id},
		{"dns_key",  key}
	};

	map<string,string> headers = {
		{"token", token}
	};

	this->CurlSetHeaders(headers);

	string body = this->DoPost("dns_addkey.php", postargs);

	Json::Value retobj = Json::objectValue;
	this->reader.parse(body, retobj);

	return this->result_code == 200;
}

bool DnsServer::UpdateDynDNS(const string &unit_id, const string &name)
{
    map<string,string> postargs;

    if ( unit_id.length() ) {
        // use normal login method
        if( ! this->Auth( unit_id ) )
        {
            return false;
        }
        logg << Logger::Debug << "Update DNS pointer"<< lend;

        postargs = {
            {"unit_id", unit_id},
            {"fqdn",  name},
            {"local_ip", NetUtils::GetAddress(sysinfo.NetworkDevice())}
        };

        map<string,string> headers = {
            {"token", this->token}
        };

        this->CurlSetHeaders(headers);
    }
    else
    {
        // update OP DNS servers based on serial number
        logg << Logger::Debug << "Update DNS based on device serial number"<< lend;
        string domain;
        SysConfig sysconfig;

        if ( sysconfig.HasKey("hostinfo","domain") )
        {
            domain=sysconfig.GetKeyAsString("hostinfo","domain");
        }
        else
        {
            domain=sysinfo.Domains[sysinfo.Type()];
        }
        postargs = {
            {"fqdn",  sysinfo.SerialNumber() + "." + domain},
            {"local_ip", NetUtils::GetAddress(sysinfo.NetworkDevice())}
        };

    }


	string body = this->DoPost("update_dns.php", postargs);

	Json::Value retobj = Json::objectValue;
	this->reader.parse(body, retobj);

	return this->result_code == 200;
}

DnsServer::~DnsServer()
{

}

bool DnsServer::Auth(const string &unit_id)
{
	try
	{
		string challenge;
		int resultcode;

		tie(resultcode,challenge) = this->GetChallenge( unit_id);

		if( resultcode != 200 )
		{
			logg << Logger::Error << "Unknown reply of server "<<resultcode<< lend;
			return false;
		}


		RSAWrapper dnskeys;
		list<string> rows = File::GetContent( SysConfig().GetKeyAsString("dns", "dnsauthkey") );
		stringstream pemkey;

		for( const auto& row: rows)
		{
			pemkey << row << "\n";
		}

		dnskeys.LoadPrivKeyFromPEM( pemkey.str() );

		string signedchallenge = Base64Encode( dnskeys.SignMessage( challenge ) );

		Json::Value rep;
		tie(resultcode, rep) = this->SendSignedChallenge( unit_id, signedchallenge );

		if( resultcode != 200 && resultcode != 403 )
		{
			logg << Logger::Error << "Unexpected reply from server "<< resultcode <<lend;
			return false;
		}

		if( resultcode == 403 )
		{
			logg << Logger::Debug << "Failed to auth with dns"<<lend;
			logg << "Reply:"<< rep.toStyledString()<<lend;
			return false;
		}

		if( rep.isMember("token") && rep["token"].isString() )
		{
			this->token = rep["token"].asString();
		}
		else
		{
			logg << Logger::Error << "Missing argument in reply"<<lend;
			return false;
		}
	}
	catch(std::exception& err)
	{
		logg << Logger::Error << "Failed to dns authenticate "<<err.what()<<lend;
		return false;
	}

	return true;
}

tuple<int, string> DnsServer::GetChallenge(const string &unit_id)
{
	string ret = "";
	map<string,string> arg = {{ "unit_id", unit_id }};

	string s_res = this->DoGet("auth.php", arg);

	Json::Value res;
	if( this->reader.parse(s_res, res) )
	{
		if( res.isMember("challange") && res["challange"].isString() )
		{
			ret = res["challange"].asString();
		}
	}
	return tuple<int,string>(this->result_code,ret);
}

tuple<int, Json::Value> DnsServer::SendSignedChallenge(const string &unit_id, const string &challenge)
{
	Json::Value data;
	data["unit_id"] = unit_id;
	data["dns_signature"] = challenge;

	map<string,string> postargs = {
		{"data", this->writer.write(data) }
	};

	string body = this->DoPost("auth.php", postargs);

	Json::Value retobj = Json::objectValue;
	this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

} // End NS
