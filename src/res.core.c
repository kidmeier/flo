#include <sys/stat.h>
#include <sys/types.h>

#include <curl/curl.h>
#include <pthread.h>

#include "core.alloc.h"
#include "res.core.h"

// Buffers ////////////////////////////////////////////////////////////////////
typedef struct {

	size_t   size;
	void*    data;

	off_t    pos;

} buf_t;

#define DEFAULT_BLK_SIZE  1024*1024
static buf_t* alloc_buf(int blk_size) {
	
	buf_t* buf = new(NULL, buf_t);
	buf->size = blk_size <= 0 ? DEFAULT_BLK_SIZE : blk_size;
	buf->data = malloc( buf->size );
	buf->pos = 0;

	return buf;
}

static size_t resize_buf( buf_t* buf, int sz ) {

	// Next highest multiple of buf->size * 2
	int   new_size = 2 * (buf->size + sz - (buf->size % sz));
	void* new_data = realloc( buf->data, new_size );

		if( !new_data )
			return -1;
		
		buf->size = new_size;
		buf->data = new_data;

		return buf->size;
}

static void free_buf( buf_t* buf ) {

	if( !buf ) return;
	if( buf->data ) free(buf->data);
	free(buf);

}

// libcurl ////////////////////////////////////////////////////////////////////

static pthread_once_t init_libcurl_once = PTHREAD_ONCE_INIT;
static void init_libcurl( void ) { curl_global_init( CURL_GLOBAL_ALL ); }
static __thread CURL* curl = NULL;

static size_t write_curldata_func( void* ptr, size_t sz, size_t n, void* arg ) {

	buf_t* buf = (buf_t*)arg;
	
	if( buf->pos + n*sz >= buf->size ) {

		if( resize_buf( buf, buf->pos + n*sz ) < 0 ) {
			memcpy( buf->data + buf->pos, ptr, buf->size - buf->pos );
			return buf->size - buf->pos;
		}

	}

	memcpy( buf->data + buf->pos, ptr, n*sz );
	buf->pos = buf->pos + n*sz;

	return n*sz;
}

static CURLcode read_url( const char* url, buf_t* buf ) {

	pthread_once( &init_libcurl_once, init_libcurl );

	if( NULL == curl ) 
		curl = curl_easy_init();

	curl_easy_setopt( curl, CURLOPT_WRITEDATA, buf );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_curldata_func );
	curl_easy_setopt( curl, CURLOPT_URL, url );

	return curl_easy_perform(curl);

}

// Public API /////////////////////////////////////////////////////////////////

struct resource_loader_s {

	char*           ext;
	load_resource_f loadfunc;

	struct resource_loader_s* next;

};

struct resource_s {

	char*  url;

	size_t size;
	any    data;

	unsigned int   refcount;
	struct timeval timestamp;
	struct timeval expiry;

	resource_p prev, next;
};

static struct resource_loader_s* loaders = NULL;
static resource_p cache = NULL;

// Caching ////////////////////////////////////////////////////////////////////

static void insert_cache( resource_p res ) {

	res->prev = NULL;
	res->next = cache;
	cache->prev = res;
	cache = res;
	
}

static void evict_cache( resource_p res ) {

	if( res->prev ) res->prev->next = res->next;
	if( res->next ) res->next->prev = res->prev;

}

static resource_p hit_cache( const char* url ) {

	resource_p node = cache;
	while( node ) {

		if( 0 == strcmp(url, node->url) )
			return node;

		node = node->next;

	}

	return NULL;

}

void register_loader_RES( const char* ext, load_resource_f loadfunc ) {

	struct resource_loader_s* ldr = new( NULL, struct resource_loader_s );

	ldr->ext = clone_string(ldr, ext);
	ldr->loadfunc = loadfunc;

	// insert into list
	ldr->next = loaders;
	loaders = ldr;

}

resource_p create_raw_RES( int size, void* data, int64 expiry ) {

	resource_p res = new( NULL, resource_t );

	res->size = size;
	res->data = data;
	adopt(res, res->data);

	// Timestamp it
	gettimeofday( &res->timestamp, NULL );

	// Compute the expiry if given, else set to 0
	if( expiry > 0 ) {
		gettimeofday( &res->expiry, NULL );
		
		res->expiry.tv_usec += 1000 * expiry;
		while( res->expiry.tv_usec >= 1000000 ) {
			res->expiry.tv_sec++;
			res->expiry.tv_usec -= 1000000;
		}
	} else
		res->expiry = (struct timeval){ 0, 0 };
	
	res->refcount = 0;

}

resource_p load_RES( const char* url, int size_hint ) {

	const char* ext = strrchr(url, '.') + 1;
	struct resource_loader_s* ldr = loaders;

	while( ldr ) {

		if( 0 == strcmp(ext, ldr->ext) )
			break;

		ldr = ldr->next;

	}

	if( ldr ) {

		resource_p res = NULL;
		buf_t* buf = alloc_buf( size_hint );
		
		if( 0 == read_url( url, buf ) )
			res = ldr->loadfunc( buf->pos, buf->data );
		
		if( res ) {
			
			res->url = clone_string( res, url );
			res->refcount++;

			insert_cache(res);

		}

		free_buf(buf);

		return res;
	}

	return NULL;

}

resource_p get_RES( const char* url ) {

	resource_p res = hit_cache(url);
	if( !res )
		return load_RES( url, -1 );

	res->refcount++;
	return res;

}

void       put_RES( resource_p res ) {

	res->refcount--;

	if( 0 == res->refcount ) {

		struct timeval t; gettimeofday(&t, NULL);
		if( t.tv_sec > res->expiry.tv_sec
		    || ( t.tv_sec == res->expiry.tv_sec && t.tv_usec >= res->expiry.tv_usec ) ) {

			evict_cache(res);
			delete(res);
		}
		
	}

}
