#include "AuthServer.h"
#include "Secop.h"
#include <stdexcept>
#include <utility>

#include <libutils/FileUtils.h>
#include <libutils/HttpStatusCodes.h>

using namespace std;
using namespace Utils::HTTP;
namespace OPI
{


AuthServer::AuthServer(string unit_id, const AuthCFG &cfg): HttpClient( cfg.authserver ), unit_id(std::move(unit_id)), acfg(cfg)
{

}

tuple<int, string> AuthServer::GetChallenge()
{
	string ret = "";
	map<string,string> arg = {{ "unit_id", this->unit_id }};

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

tuple<int, Json::Value> AuthServer::SendSignedChallenge(const string &challenge)
{
	Json::Value data;
	data["unit_id"] = this->unit_id;
	data["signature"] = challenge;

	map<string,string> postargs = {
		{"data", this->writer.write(data) }
	};

	Json::Value retobj = Json::objectValue;
	string body = this->DoPost("auth.php", postargs);

    this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

tuple<int, Json::Value> AuthServer::Login(bool usetempkeys)
{
	Json::Value ret;
	CryptoHelper::RSAWrapperPtr c;

	try
	{
		// Secop operations might throw an exception
		if( usetempkeys )
		{
			c = AuthServer::GetKeysFromFile(this->acfg.pubkeypath, this->acfg.privkeypath);
		}
		else
		{
			c = AuthServer::GetKeysFromSecop();
		}
	}
	catch (std::runtime_error& err)
	{
		ret["desc"]=string("Failed to retrieve keys: ")+err.what();
		return tuple<int, Json::Value>(Status::ServiceUnavailable, ret);
	}

	if( ! c )
	{
		ret["desc"]="Failed to get retrieve keys";
		return tuple<int, Json::Value>(Status::ServiceUnavailable, ret);
	}

	string challenge;
	int resultcode = 0;
	tie(resultcode,challenge) = this->GetChallenge();

	if( resultcode != Status::Ok )
	{
		ret["desc"] = "Unknown reply from server";
		ret["value"] = resultcode;
		return tuple<int, Json::Value>(Status::InternalServerError, ret);
	}

	string signedchallenge = CryptoHelper::Base64Encode( c->SignMessage( challenge ) );

	Json::Value rep;
	tie(resultcode, rep) = this->SendSignedChallenge( signedchallenge );

	if( resultcode != Status::Ok )
	{
		ret["desc"] = "Unexpected reply from server";
		ret["value"] = resultcode;
		ret["reply"] = rep;
		return tuple<int, Json::Value>(resultcode, ret);
	}

	if( rep.isMember("token") && rep["token"].isString() )
	{
		ret["token"] = rep["token"].asString();
		return tuple<int, Json::Value>(Status::Ok, ret);
	}

	ret["desc"] = "Malformed reply from server";
	return tuple<int, Json::Value>(Status::InternalServerError, ret);
}

tuple<int, Json::Value> AuthServer::SendSecret(const string &secret, const string &pubkey)
{
	Json::Value data;
	data["unit_id"] = this->unit_id;
	data["response"] = secret;
	data["PublicKey"] = pubkey;

	map<string,string> postargs = {
		{"data", this->writer.write(data) }
	};

	string body = this->DoPost("register_public.php", postargs);

	Json::Value retobj = Json::objectValue;
	if( ! this->reader.parse(body, retobj) )
	{
		retobj = Json::objectValue;
		retobj["error"]=body;
	}

	return tuple<int,Json::Value>(this->result_code, retobj );
}

tuple<int, Json::Value> AuthServer::GetCertificate(const string &csr, const string &token)
{
	map<string,string> postargs = {
		{"unit_id", this->unit_id },
		{"csr", csr }
	};

	map<string,string> headers = {
		{"token", token}
	};

	this->CurlSetHeaders(headers);

	Json::Value retobj = Json::objectValue;
	string body = this->DoPost("get_cert.php", postargs);

	this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

tuple<int, Json::Value> AuthServer::UpdateMXPointer(bool useopi, const string &token)
{
	map<string,string> postargs = {
		{"unit_id", this->unit_id },
		{"enable_mx", useopi?"1":"0" }
	};

	map<string,string> headers = {
		{"token", token}
	};

	this->CurlSetHeaders(headers);

	Json::Value retobj = Json::objectValue;
	string body = this->DoPost("update_mx.php", postargs);

	this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

tuple<int, Json::Value> AuthServer::CheckMXPointer(const string &name)
{
	map<string,string> postargs = {
		{"fqdn", name },
		{"test_mx", "1" },
		{"type", "MX" }
	};

	Json::Value retobj = Json::objectValue;
	string body = this->DoPost("update_mx.php", postargs);

	this->reader.parse(body, retobj);

	return tuple<int,Json::Value>(this->result_code, retobj );
}

void AuthServer::Setup()
{
	Secop s;

	s.SockAuth();
	list<map<string,string>> ids;

	try
	{
		ids = s.AppGetIdentifiers("op-backend");
	}
	catch( __attribute__((unused)) runtime_error& err )
	{
		// Do nothing, appid is missing but thats ok.
	}

	if( ids.size() == 0 )
	{
		s.AppAddID("op-backend");

		RSAWrapper ob;
		ob.GenerateKeys();

		// Write to secop
		map<string,string> data;

		data["type"] = "backendkeys";
		data["pubkey"] = Base64Encode(ob.GetPubKeyAsDER());
		data["privkey"] = Base64Encode(ob.GetPrivKeyAsDER());
		s.AppAddIdentifier("op-backend", data);
	}

}

RSAWrapperPtr AuthServer::GetKeysFromSecop()
{
	CryptoHelper::RSAWrapperPtr c;

	Secop secop;
	secop.SockAuth();

	list<map<string,string>> ids =  secop.AppGetIdentifiers("op-backend");

	if( ids.size() == 0 )
	{
		return c;
	}

	for(auto id : ids )
	{
		if( id.find("type") != id.end() )
		{
			if( id["type"] == "backendkeys" )
			{
				c = RSAWrapperPtr(new RSAWrapper);
				// Key found
				c->LoadPrivKeyFromDER( CryptoHelper::Base64Decode( id["privkey"]) );
				c->LoadPubKeyFromDER( CryptoHelper::Base64Decode( id["pubkey"]) );
				break;
			}
		}
	}

	return c;
}

RSAWrapperPtr AuthServer::GetKeysFromFile(const string &pubpath, const string &privpath)
{
	CryptoHelper::RSAWrapperPtr c( new RSAWrapper );

	string pub_pem = Utils::File::GetContentAsString( pubpath, true );
	string priv_pem = Utils::File::GetContentAsString( privpath, true );

	c->LoadPubKeyFromPEM( pub_pem );
	c->LoadPrivKeyFromPEM( priv_pem );

	return c;
}

AuthServer::~AuthServer() = default;

} // End NS
