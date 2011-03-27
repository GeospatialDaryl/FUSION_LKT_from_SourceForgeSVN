// filespec.h class definition for file specifier class

#ifndef _FILESPEC
#define _FILESPEC

#include <boost/filesystem.hpp>

#include "vc6_to_std.h"

class CFileSpec
{
public:
	LPCTSTR operator=(LPCTSTR spec);

	void SetDirectory( LPCTSTR directory );
	LPCTSTR GetShortPathName();
	void SetTitle( LPCTSTR title ) {
		   CString FileName = title;
		   FileName  += Extension();
		   SetFileNameEx( FileName);
	}

	enum FS_BUILTINS {
			FS_EMPTY,			//   Nothing
			FS_APP,				//   Full application path and name
			FS_APPDIR,			//   Application folder
			FS_WINDIR,			//   Windows folder
			FS_SYSDIR,			//   System folder
			FS_TMPDIR,			//   Temporary folder
			FS_DESKTOP,			//       Desktop folder
			FS_FAVOURITES,		//       Favourites folder
			FS_TEMPNAME			//       Create a temporary name
	};

	CFileSpec(FS_BUILTINS spec = FS_EMPTY);

	CFileSpec(LPCTSTR spec);

	//      Operations
	BOOL                    Exists();

	//      Access functions
	CString     Drive() { return path_.root_name(); }
	CString     Path()  { return path_.parent_path().relative_path().string(); }
	CString     Directory() { return path_.parent_path().string(); }
	CString     FileName()  { return path_.filename(); }
	CString     FileTitle() { return path_.stem(); }
	CString     Extension() { return path_.extension(); }
	CString     FullPathNoExtension();

	operator LPCTSTR() { 
		return path_.string().c_str();
	}          // as a C string

	CString GetFullSpec();
	void    SetFullSpec(LPCTSTR spec);
	void    SetFullSpec(FS_BUILTINS spec = FS_EMPTY);
       
	CString GetFileNameEx();
	void    SetFileNameEx(LPCTSTR spec);
	void    SetExt( LPCTSTR Ext ) {
	   path_.replace_extension(Ext);
	}
	void    SetDrive( LPCTSTR Drive ) {		// Drive letter and colon
     CString newPath = Drive;
	   newPath += Path();
	   newPath += FileName();
     path_ = newPath;
	}

private:
	void    Initialize(FS_BUILTINS spec);

	boost::filesystem::path path_;
};

#endif

