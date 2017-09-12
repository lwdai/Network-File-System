#include "client_fuse_ops.h"
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <vector>
#include "client_grpc.h"

using namespace std;

ClientGrpc* client;

void *nfs_init(struct fuse_conn_info *conn,
			   struct fuse_config *cfg) {
    printf("nfs_init\n");
    (void) conn;
	cfg->kernel_cache = 1;
	return NULL;			   			   
} // nfs_init    	

int nfs_getattr(const char *path, struct stat *stbuf,
			    struct fuse_file_info *fi) {
	//printf("nfs_getattr: %s\n", path);
	
	int err = client->send_getattr_request( path, stbuf );
	
	//cerr << "err = " << err << endl;
	return err;		  
} // nfs_getattr
			    
int nfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags) {
			 
	//printf("nfs_readdir: %s\n", path);
	(void) offset;
	(void) fi;
	(void) flags;

    // fetch a list of dirent, fill to buf
	vector<dirent> dirs;
	
	int err = client->send_readdir_request( path, dirs );
	if ( 0 != err ) {	    
	    return err;
	} // if
	for ( unsigned int i = 0; i < dirs.size(); i++ ) {
	    struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = dirs[i].d_ino;
		st.st_mode = dirs[i].d_type << 12;
		if (filler(buf, dirs[i].d_name, &st, 0, 
		    static_cast<fuse_fill_dir_flags>(0))) {
		    break;
		} // if
	} // for
	
	return 0;
} // nfs_readdir
			
int nfs_open(const char *path, struct fuse_file_info *fi) {
    //printf("nfs_open: %s\n", path);
    
	return client->send_open_request( path, fi->flags, fi->fh );

} // nfs_open



int nfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
	//printf( "nfs_read: %s, size=%d, offset=%d\n", path, (int)size, (int)offset );
	
	int err = -1;
	
	int len = client->send_read_request( path, buf, err, size, offset, fi->fh );
	
	if ( -1 == len ) return err;
	return len;
	      
} // nfs_read


int nfs_mkdir( const char *path, mode_t mode ) {
    //printf("nfs_mkdir: %s\n", path );
    
    return client->send_mkdir_request( path, mode );

} // nfs_mkdir

int nfs_rmdir( const char *path ) {
    //printf("nfs_rmdir: %s\n", path );
    
    return client->send_rmdir_request( path );

} // nfs_rmdir

int nfs_create( const char *path, mode_t mode, struct fuse_file_info *fi ) {
    //printf( "nfs_create: %s\n", path );
    
    return client->send_create_request( path, mode, fi->flags, fi->fh );
} // nfs_create

int nfs_write( const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi ) {
	//printf( "nfs_write: %s, size=%d, offset=%d\n", path, (int)size, (int)offset );	
	
	return client->send_write_request( path, buf, size, offset, fi->fh );   
} // nfs_write

int nfs_unlink( const char *path ) {
    //printf("nfs_unlink: %s\n", path );
    return client->send_unlink_request( path );
} // nfs_unlink

int nfs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi) {
    (void) fi;
    
    return client->send_truncate_request( path, size, fi->fh );			
} // nfs_truncate

int nfs_rename(const char *from, const char *to, unsigned int flags) {
    if ( flags ) return -EINVAL;
    
    return client->send_rename_request( from, to );
} // nfs_rename


int nfs_flush( const char *path, struct fuse_file_info *fi ) {
    return client->send_flush_request( path, fi->fh );
} // nfs_flush


static struct fuse_operations nfs_oper;

void setup_ops() {
    nfs_oper.init = nfs_init;
    nfs_oper.getattr = nfs_getattr;
    nfs_oper.readdir = nfs_readdir;
    nfs_oper.open = nfs_open;
    nfs_oper.read = nfs_read;
    nfs_oper.write = nfs_write;
    nfs_oper.mkdir = nfs_mkdir;
    nfs_oper.rmdir = nfs_rmdir;
    nfs_oper.create = nfs_create;
    nfs_oper.unlink = nfs_unlink;
    nfs_oper.truncate = nfs_truncate;
    nfs_oper.rename = nfs_rename;
    //nfs_oper.flush = nfs_flush;
    
    // well, name abused..
    nfs_oper.release = nfs_flush;
} // setup_ops
void parse_config( string &server_port, string &server_host, const char *config_file ) {
    ifstream in;
    in.open( config_file );
    
    string key;
    
    while ( in >> key ) {
        if ( key == "ServerPort" ) {
            in >> server_port;
        }
        if( key == "ServerHost"){
          in >> server_host;
        }
    }
    
    in.close();
} // parse_config

int main(int argc, char*argv[]){
	string server_port;
  	string server_host;
  	parse_config(server_port, server_host, "client.config");
  	string server_address = server_host+":"+server_port;
	client = new ClientGrpc(grpc::CreateChannel(
				server_address, grpc::InsecureChannelCredentials()));
	
	setup_ops();
	umask(0);
	return fuse_main(argc, argv, &nfs_oper, NULL);
}

