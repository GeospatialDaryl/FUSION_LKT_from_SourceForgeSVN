#include <sstream>
#include <stdexcept>

#include "getApplicationPath.h"
#include "FileSpec.h"

CFileSpec::CFileSpec(FS_BUILTINS spec)
{
	Initialize(spec);
}

CFileSpec::CFileSpec(LPCTSTR spec)
{
	SetFullSpec(spec);
}

void CFileSpec::Initialize(FS_BUILTINS spec)
{
	path_ = "";

	switch (spec) {
	case FS_EMPTY:                                              //      Nothing
		break;

	case FS_APP:                                                //      Full application path and name
    path_ = getApplicationPath();
		break;

	case FS_APPDIR:                                             //      Application folder
    path_ = getApplicationPath();
    path_.remove_filename();
		break;

	case FS_WINDIR:                                             //      Windows folder
    throw std::runtime_error("CFileSpec::FS_WINDIR not implemented");

	case FS_SYSDIR:                                             //      System folder
    throw std::runtime_error("CFileSpec::FS_SYSDIR not implemented");

	case FS_TMPDIR:                                             //      Temporary folder
    throw std::runtime_error("CFileSpec::FS_TMPDIR not implemented");

	case FS_DESKTOP:                                            //      Desktop folder
    throw std::runtime_error("CFileSpec::FS_DESKTOP not implemented");

	case FS_FAVOURITES:                                         //      Favourites folder
    throw std::runtime_error("CFileSpec::FS_FAVOURITES not implemented");

	case FS_TEMPNAME:
    throw std::runtime_error("CFileSpec::FS_TEMPNAME not implemented");

	default:
    std::ostringstream message;
		message << "Invalid initialization spec for CFileSpec, " << spec;
		throw std::runtime_error(message.str());
	}
}

//      Operations
BOOL CFileSpec::Exists()
{
	return boost::filesystem::exists(path_);
}

//      Access functions : Get title + ext
CString CFileSpec::GetFileNameEx()
{
	return FileName();  // Not sure why this functionality was duplicated
}

void CFileSpec::SetFileNameEx(LPCTSTR spec)
{
  std::string newFileNameExt(spec);
  path_ = path_.parent_path() / newFileNameExt;
}

CString CFileSpec::FullPathNoExtension()
{
  boost::filesystem::path pathNoExt = path_.parent_path() / path_.stem();
	return pathNoExt.string();
}

CString CFileSpec::GetFullSpec()
{
	return path_.string();
}

void CFileSpec::SetFullSpec(FS_BUILTINS spec)
{
	Initialize(spec);
}

void CFileSpec::SetFullSpec(LPCTSTR spec)
{
  path_ = spec;
}

LPCTSTR CFileSpec::GetShortPathName()
{
  throw std::runtime_error("GetShortPathName not implemented because not cross-platform");
}

void CFileSpec::SetDirectory(LPCTSTR directory)
{
	CString FullSpec = directory;

	if ( strlen(FileName()) == 0 ) {
		// this is section is used in case the Directory is set before the
		// filename
		SetFileNameEx( "temp");
		FullSpec += FileName();
		SetFullSpec (FullSpec );
		SetFileNameEx( "");
	}
	else {
		FullSpec += FileName();
		SetFullSpec (FullSpec );
	}
}


LPCTSTR CFileSpec::operator=(LPCTSTR spec)
{
	SetFullSpec(spec);
	return GetFullSpec();
}


