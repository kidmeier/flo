#include <string.h>
#include "core.string.h"

int maybe_strncpy( char* dest, int n, const char* src ) {

	int len = strlen(src);
	if( len+1 > n )
		return -(len+1);

	strncpy( dest, src, n );
	return len+1;

}
