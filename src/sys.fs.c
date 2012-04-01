#include <string.h>

#include "sys.fs.h"

#if defined( feature_POSIX ) || defined( feature_MINGW )

bool Fs_exists( const char *path ) {

	struct stat statbuf;

	if( stat(path, &statbuf) < 0 )
		return false;
	else
		return true;

}

int Fs_mkdirs( const char *path ) {

	char workpath[ strlen(path)+1 ];
	strcpy( workpath, path );
	
	for( char *sep = strchr( workpath+1, fileSeparator ); sep; sep = strchr(sep+1, fileSeparator) ) {
		
		*sep = '\0';

		int ret = Fs_exists(workpath) 
			? 0 
#if defined( feature_POSIX )
			: mkdir( workpath, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH );
#elif defined( feature_MINGW )
			: mkdir( workpath );
#endif

		if( ret < 0 )
			return ret;

		*sep = fileSeparator;

	}
	
	if( !Fs_exists(workpath) )
#if defined( feature_POSIX )
		return mkdir( workpath, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH );
#elif defined( feature_MINGW )
		return mkdir( workpath );
#endif

	return 0;

}

#elif defined( feature_WIN32 )

bool Fs_exists( const char *path ) {
	return false;
}

#endif
