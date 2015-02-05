#include "BackupHelper.h"

#include <sstream>
#include <tuple>

#include <ext/stdio_filebuf.h>
#include <libutils/Exceptions.h>
#include <libutils/FileUtils.h>
#include <libutils/Process.h>
#include <unistd.h>

#define MOUNTCMD	"/usr/share/opi-backup/mount_fs.sh"
#define UMOUNTCMD	"/usr/share/opi-backup/umount_fs.sh"
#define RESTORECMD	"/usr/share/opi-backup/restore_backup.sh"
#define LOCALMOUNT	"/tmp/localbackup"
#define REMOTEMOUNT	"/tmp/remotebackup"

namespace OPI
{

BackupHelper::BackupHelper(const string& pwd, BackupInterfacePtr iface):
	localmounted(false), remotemounted(false), cfgcreated(false),
	pwd(pwd), iface(iface)
{

}

void BackupHelper::SetPassword(const string &pwd)
{
	this->pwd = pwd;
	unlink( this->tmpfilename );
	this->cfgcreated = false;
}

bool BackupHelper::MountLocal()
{
	this->CreateConfig();

	if( ! this->localmounted )
	{
		this->localmounted  = this->iface->MountLocal( this->tmpfilename );
	}
	return this->localmounted;
}

bool BackupHelper::MountRemote( )
{
	this->CreateConfig();

	if ( ! this->remotemounted )
	{
		this->remotemounted = this->iface->MountRemote( this->tmpfilename );
	}
	return this->remotemounted;
}

list<string> BackupHelper::GetLocalBackups()
{
	if( this->localmounted )
	{
		return this->iface->GetLocalBackups();
	}
	return {};
}

list<string> BackupHelper::GetRemoteBackups()
{
	if( this->remotemounted )
	{
		return this->iface->GetRemoteBackups();
	}
	return {};
}

void BackupHelper::UmountLocal()
{
	this->iface->UmountLocal();
	this->localmounted = false;
}

void BackupHelper::UmountRemote()
{
	this->iface->UmountRemote();
	this->remotemounted = false;
}

bool BackupHelper::RestoreBackup(const string &path)
{
	return this->iface->RestoreBackup( path );
}

BackupHelper::~BackupHelper()
{
	if( this->localmounted )
	{
		this->UmountLocal();
	}

	if( this->remotemounted )
	{
		this->UmountRemote();
	}

	if( this->cfgcreated )
	{
		unlink( this->tmpfilename );
	}
}

void BackupHelper::CreateConfig()
{
	if( this->cfgcreated )
	{
		return;
	}

	sprintf( this->tmpfilename,"/tmp/bkcfgXXXXXX");

	int fd = mkstemp(this->tmpfilename);

	if( fd <0 )
	{
		throw Utils::ErrnoException("Failed to create file");
	}

	__gnu_cxx::stdio_filebuf<char> fb( fd, std::ios::out);

	ostream out(&fb);

	out <<"[s3op]\n"
		<<"storage-url: s3op://\n"
		<<"backend-login: NotUsed\n"
		<<"backend-password: NotUsed\n"
		<<"fs-passphrase: "<< this->pwd<<"\n"
		<<"\n"
		<< "[local]\n"
		<< "storage-url: local://\n"
		<< "fs-passphrase: "<< this->pwd << "\n";

	out << flush;

	this->cfgcreated = true;
}

string BackupHelper::GetConfigFile()
{
	return this->tmpfilename;
}

OPIBackup::OPIBackup()
{

}

static bool trymount(const string& path, bool local)
{
	bool result;

	stringstream ss;

	ss << MOUNTCMD << " -r -a " << path << " -b ";
	if( local )
	{
		ss << "\"local://\" -m " << LOCALMOUNT;
	}
	else
	{
		ss << "\"s3op://\" -m " << REMOTEMOUNT;
	}

	tie(result, std::ignore ) = Utils::Process::Exec( ss.str() );

	return result;
}

bool OPIBackup::MountLocal(const string &configpath)
{
	if( ! Utils::File::DirExists( LOCALMOUNT ) )
	{
		Utils::File::MkDir( LOCALMOUNT, 0700);
	}

	return trymount( configpath, true);
}

bool OPIBackup::MountRemote(const string &configpath)
{
	if( ! Utils::File::DirExists( REMOTEMOUNT ) )
	{
		Utils::File::MkDir( REMOTEMOUNT, 0700);
	}

	return trymount( configpath, false);
}

static list<string> getbackups(bool local)
{
	string bp;
	if( local )
	{
		bp = LOCALMOUNT "/*";
	}
	else
	{
		bp = REMOTEMOUNT "/*";
	}

	list<string> ret, res = Utils::File::Glob( bp );
	for( const auto& r: res)
	{

		if( Utils::File::GetFileName( r ) != "lost+found" )
		{
			ret.push_back( r );
		}
	}
	return ret;
}

list<string> OPIBackup::GetLocalBackups()
{
	return getbackups( true );
}

list<string> OPIBackup::GetRemoteBackups()
{
	return getbackups(false);
}


static bool doumount(bool local)
{
	bool result;

	stringstream ss;

	ss << UMOUNTCMD;
	if( local )
	{
		ss << " -m " << LOCALMOUNT;
	}
	else
	{
		ss << " -m " << REMOTEMOUNT;
	}

	tie(result, std::ignore ) = Utils::Process::Exec( ss.str() );

	return result;
}

void OPIBackup::UmountLocal()
{
	doumount( true );
}

void OPIBackup::UmountRemote()
{
	doumount( false );
}

bool OPIBackup::RestoreBackup(const string &pathtobackup)
{
	bool result;

	stringstream ss;

	ss << RESTORECMD << " \"" <<pathtobackup << "\"";

	tie(result, std::ignore ) = Utils::Process::Exec( ss.str() );

	return result;
}

OPIBackup::~OPIBackup()
{

}

}
