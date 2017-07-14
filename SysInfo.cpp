#include "SysInfo.h"
#include "Config.h"

#include <map>

#include <json/json.h>

#include <libutils/FileUtils.h>
#include <libutils/String.h>

using namespace Utils;


namespace OPI
{

SysInfo sysinfo;

SysInfo::SysInfo(): numcpus(0), type(TypeUndefined), arch(ArchUndefined)
{
	this->GuessType();
	this->SetupPaths();
}

int SysInfo::NumCpus()
{
	return this->numcpus;
}

SysInfo::SysType SysInfo::Type()
{
	return this->type;
}

SysInfo::SysArch SysInfo::Arch()
{
	return this->arch;
}

string SysInfo::StorageDevicePath()
{
	return this->storagedevicepath+"/"+this->storagedevice+this->storagepartition;
}

string SysInfo::StorageDevice()
{
	return this->storagedevicepath+"/"+this->storagedevice;
}

string SysInfo::StorageDeviceBlock()
{
	return this->storagedevice;
}

string SysInfo::StorageDevicePartition()
{
	return this->storagepartition;
}

string SysInfo::PasswordDevice()
{
	return this->passworddevicepath;
}

string SysInfo::NetworkDevice()
{
	return this->networkdevice;
}

SysInfo::~SysInfo()
{

}

bool SysInfo::isArmada()
{
	return sysinfo.Type() == SysInfo::TypeArmada;
}

bool SysInfo::isOpi()
{
	return sysinfo.Type() == SysInfo::TypeOpi;
}

bool SysInfo::isXu4()
{
	return sysinfo.Type() == SysInfo::TypeXu4;
}

bool SysInfo::isOlimexA20()
{
	return sysinfo.Type() == SysInfo::TypeOlimexA20;
}

bool SysInfo::isPC()
{
	return sysinfo.Type() == SysInfo::TypePC;
}

SysInfo::SysType SysInfo::TypeFromName(const string &devname)
{
	static map<string, SysInfo::SysType> devtypemap =
	{
		{"opi",SysInfo::TypeOpi },
		{"xu4",SysInfo::TypeXu4 },
		{"olimexa20",SysInfo::TypeOlimexA20 },
		{"armada",SysInfo::TypeArmada },
		{"pc",SysInfo::TypePC },
	};


	if( devtypemap.find(devname) != devtypemap.end() )
	{
		return devtypemap[devname];
	}
	return SysType::TypeUnknown;
}

void SysInfo::GuessType()
{
	if( ! File::FileExists("/proc/cpuinfo") )
	{
		return;
	}
	list<string> lines = File::GetContent("/proc/cpuinfo");
	for( string line: lines)
	{
		list<string> kval = String::Split(line,":");
		if( kval.size() == 2 )
		{
			string key = String::Trimmed(kval.front(), " \t");
			string val = String::Trimmed(kval.back(), " \t");

			if( key == "processor" )
			{
				this->numcpus++;
			}
			else if( key == "model name")
			{
				if( val.find("Intel") != string::npos )
				{
					this->type = SysInfo::TypePC;
					this->arch = SysInfo::ArchIntel;
				}
				if( val.find("ARMv7") != string::npos )
				{
					this->arch = SysInfo::ArchArm;
				}
			}
			else if( key == "Hardware")
			{
				if( val.find("Generic AM33XX") != string::npos )
				{
					this->type = SysInfo::TypeOpi;
				}
				else if ( val.find("ODROID-XU3") != string::npos )
				{
					this->type = SysInfo::TypeXu4;
				}
				else if ( val.find("sun7i") != string::npos )
				{
					this->type = SysInfo::TypeOlimexA20;
				}
				else if ( val.find("Marvell Armada") != string::npos )
				{
					this->type = SysInfo::TypeArmada;
				}
			}
		}
	}

	// Have we found a match?
	if( this->type == TypeUndefined )
	{
		// Nopes
		this->type = TypeUnknown;
	}
}

void SysInfo::SetupPaths()
{
	// Setup system defaults
	switch( this->type )
	{
	case TypeOpi:
		this->storagedevicepath = "/dev/disk/by-path";
		this->storagedevice = "platform-48060000.mmc";
		this->storagepartition = "-part1";
		this->passworddevicepath = "/dev/sda1";
		this->networkdevice = "eth0";
		break;
	case TypeXu4:
		this->storagedevicepath = "/dev/disk/by-path";
		this->storagedevice = "platform-xhci-hcd.2.auto-usb-0:1.1:1.0-scsi-0:0:0:0";
		this->storagepartition = "-part1";
		this->passworddevicepath = "/dev/disk/by-path/platform-12110000.usb-usb-0:1:1.0-scsi-0:0:0:0-part1";
		this->networkdevice = "eth0";
		break;
	case TypePC:
		this->storagedevicepath = "/dev";
		this->storagedevice = "sdg";
		this->storagepartition = "1";
		this->passworddevicepath = "Undefined";
		this->networkdevice = "eth0";
		break;
	case TypeArmada:
		this->storagedevicepath = "/dev/disk/by-path";
		this->storagedevice = "platform-f10a8000.sata-ata-1";
		this->storagepartition = "-part1";
		this->passworddevicepath = "/dev/disk/by-path/platform-f10f8000.usb3-usb-0:1:1.0-scsi-0:0:0:0-part1";
		this->networkdevice = "eth0";
		break;
	case TypeOlimexA20:
		this->storagedevicepath = "/dev";
		this->storagedevice = "sda";
		this->storagepartition = "1";
		this->passworddevicepath = "/dev/sdb1";
		this->networkdevice = "eth0";
		break;
	default:
		break;
	}

	if( File::FileExists(DEVICEDBPATH) )
	{
		this->ParseExtConfig();
	}
}

void SysInfo::ParseExtConfig()
{
	// Read override config
	string fil = File::GetContentAsString(DEVICEDBPATH);
	Json::Value db;


	if( Json::Reader().parse(fil, db) )
	{
		list<string> devices({"opi","xu4","olimexa20","armada","pc"});

		for(const string& dev: devices )
		{
			if( db.isMember(dev) && db[dev].isObject() )
			{
				if( this->TypeFromName(dev) == this->Type()  )
				{
					this->ParseExtEntry( db[dev]);
				}
			}
		}

	}

}

void SysInfo::ParseExtEntry(Json::Value &v)
{
	if( v.isMember("storagedevicepath") && v["storagedevicepath"].isString() )
	{
		this->storagedevicepath = v["storagedevicepath"].asString();
	}
	if( v.isMember("storagedevice") && v["storagedevice"].isString() )
	{
		this->storagedevice = v["storagedevice"].asString();
	}
	if( v.isMember("storagepartition") && v["storagepartition"].isString() )
	{
		this->storagepartition = v["storagepartition"].asString();
	}
	if( v.isMember("passworddevicepath") && v["passworddevicepath"].isString() )
	{
		this->passworddevicepath = v["passworddevicepath"].asString();
	}
	if( v.isMember("networkdevice") && v["networkdevice"].isString() )
	{
		this->networkdevice = v["networkdevice"].asString();
	}
}

} // Namespace OPI
