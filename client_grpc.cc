#include "client_grpc.h"
#include <vector>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
using namespace nfs;
using namespace std;

using nfs::Stat;
using nfs::Dirent;
using nfs::GetattrRequest;
using nfs::GetattrReply;
using nfs::ReaddirRequest;
using nfs::ReaddirReply;
using nfs::OpenRequest;
using nfs::OpenReply;
using nfs::ReadRequest;
using nfs::ReadReply;
using nfs::MkdirRequest;
using nfs::MkdirReply;
using nfs::RmdirRequest;
using nfs::RmdirReply;
using nfs::CreateRequest;
using nfs::CreateReply;
using nfs::WriteRequest;
using nfs::WriteReply;
using nfs::UnlinkRequest;
using nfs::UnlinkReply;
using nfs::TruncateRequest;
using nfs::TruncateReply;
using nfs::RenameRequest;
using nfs::RenameReply;
using nfs::FlushRequest;
using nfs::FlushReply;

static void read_stat( struct stat *dest,  const Stat &src ) {

    dest->st_dev=	src.sta_dev();
    dest->st_ino =	src.sta_ino();
    dest->st_mode=	src.sta_mode();
    dest->st_nlink=	src.sta_nlink();
    dest->st_uid=	src.sta_uid();
    dest->st_gid=	src.sta_gid();
    dest->st_rdev=	src.sta_rdev();
    dest->st_size=	src.sta_size();
    dest->st_blksize=src.sta_blksize();
    dest->st_blocks=	src.sta_blocks();
    dest->st_atime=	src.sta_atime();
    dest->st_mtime=	src.sta_mtime();
    dest->st_ctime=	src.sta_ctime();

} // read_stat


/**
 * Returns 0 on success; and -1 or -errno on failure
 */ 
int 
ClientGrpc::send_getattr_request( const char* path, struct stat* stbuf ) {

    GetattrRequest req;
    req.set_path( path );
        
	GetattrReply reply;
    ClientContext context;

    //The actual RPC.
    Status status = stub_->getattr_request(&context, req, &reply);
       
    if(status.ok()) {
    //std::cout << "send_getattr_request rpc succeeded." << std::endl;
    	read_stat( stbuf, reply.attr() );
    	return reply.err();    	
    }
    
    cerr << "send_getattr_request rpc failed: " << status.error_code() 
    	          << ": " << status.error_message() << std::endl;
    return -1;
} // send_getattr_request

// copy the code above; just change request & reply type
int
ClientGrpc::send_readdir_request( const char* path, vector<dirent> &dirs ) {

    ReaddirRequest req;
    req.set_path( path );
    
	ReaddirReply reply;

    ClientContext context;
    
    // rpc
    Status status = stub_->readdir_request(&context, req, &reply);

    if (status.ok()) {        
    	//std::cout << "send_readdir_request rpc succeeded." << std::endl;

    	// copy out all dirents
    	for ( int i = 0; i < reply.de_size(); i++ ) {
    	    dirent d;
    	    d.d_ino = reply.de(i).d_ino();
    	    d.d_type = reply.de(i).d_type();
    	    strcpy( d.d_name, reply.de(i).d_name().c_str() );
    	        
    	    dirs.push_back( d );
    	} // for
    	        	    
    	return reply.err();
    } // if
    
    cerr << "send_readdir_request rpc failed: " << status.error_code() << endl;
    return -1;
} // send_readdir_request


// copy the code above; just change request & reply type
// Returns the 0 or -errno
int ClientGrpc::send_open_request(const char* path, const int flags, uint64_t &fh ) {
    OpenRequest req;
    req.set_path( path );
    req.set_flags( flags );
    
	OpenReply reply;
    ClientContext context;

    // rpc
    Status status = stub_->open_request(&context, req, &reply);

    if(status.ok()) {    	
    	//cerr << "send_open_request rpc succeeded." << std::endl;
    	fh = reply.fh();
    	return reply.err();
    } // if
    
    cerr << "send_open_request rpc failed: " << status.error_code() 
    	          << ": " << status.error_message() << std::endl;
    return -1;
} // send_open_request

// copy the code above; just change request & reply type
// Returns number of bytes read, or -1 on error
int ClientGrpc::send_read_request(const char* path, char *buf, int &error,
    const size_t size, const off_t offset, uint64_t fh ) {
    
    ReadRequest req;
    //req.set_path( path );
    (void) path;
    req.set_size( size );
    req.set_offset( offset );
    req.set_fh( fh );

	ReadReply reply;

    ClientContext context;

    // rpc
    Status status = stub_->read_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_read_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    error = reply.err();
    	    return -1;
    	} // if
    	// copy out data to buf
  	
    	int len = reply.data().size();
    	memcpy( buf, reply.data().c_str(), len );
    	return len;

    } // if
    
    cerr << "send_read_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
} // send_read_request


// Returns 0 on success and -errno on failure
int ClientGrpc::send_mkdir_request( const char *path, mode_t mode ) {
    MkdirRequest req;
    req.set_path( path );
    // mode_t is int ?
    req.set_mode( mode );
    
    MkdirReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->mkdir_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_mkdir_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_mkdir_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
} // send_mkdir_request

// Returns 0 on success and -errno on failure
int ClientGrpc::send_rmdir_request( const char *path ) {
    RmdirRequest req;
    req.set_path( path );
    
    RmdirReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->rmdir_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_rmdir_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_rmdir_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
} // send_mkdir_request


// Returns 0 on success and -errno on failure
int ClientGrpc::
    send_create_request( const char *path, mode_t mode, const int flags, uint64_t &fh ) {
    CreateRequest req;
    req.set_path( path );
    req.set_mode( mode );
    req.set_flags( flags );
    
    CreateReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->create_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_create_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {    	   
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	fh = reply.fh();
    	return 0;
    } // if
    
    cerr << "send_create_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
    
} // send_create_request

// On success, returns number of bytes written; on failure returns -1 or -errno;
int ClientGrpc::
    send_write_request( const char *path, const char *buf, 
                        size_t size, off_t offset, uint64_t fh ) {
    WriteRequest req;
    //req.set_path( path );
    (void) path;
    req.set_offset( offset );
    req.set_data( buf, size );
    req.set_fh( fh );
    
    WriteReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->write_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_write_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return reply.len();
    } // if
    
    cerr << "send_write_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;                    
} // send_write_request


// Returns 0 on success, and -errno on failure
int ClientGrpc::
    send_unlink_request( const char *path ) {
    
    UnlinkRequest req;
    req.set_path( path );
    
    UnlinkReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->unlink_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_unlink_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_unlink_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;

} // send_unlink_request


// Returns 0 on success and -errno on failure
int ClientGrpc::
send_truncate_request( const char *path, off_t size, uint64_t fh ) {
    TruncateRequest req;
    //req.set_path( path );
    (void) path;
    req.set_size( size );
    req.set_fh( fh );
    
    TruncateReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->truncate_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_truncate_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_truncate_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
} // send_truncate_request

// Returns 0 on success and -errno on failure
int ClientGrpc::        
send_rename_request( const char *from, const char *to ) {
    RenameRequest req;
    req.set_from_name( from );
    req.set_to_name( to );
    
    RenameReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->rename_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_rename_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_rename_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;
} // send_rename_request

// Returns the 0 on success and -errno on failure
int ClientGrpc::
send_flush_request( const char *path, uint64_t fh ) {
    FlushRequest req;
    //req.set_path( path );
    (void) path;
    req.set_fh( fh );
    
    FlushReply reply;
    
    ClientContext context;
    
    // rpc
    Status status = stub_->flush_request(&context, req, &reply);

    if ( status.ok() ) {
    	//std::cout << "send_flush_request rpc succeeded." << std::endl;
    	
    	if ( reply.err() != 0 ) {
    	    return reply.err();
    	} // if
    	// copy out data to buf
    	
    	return 0;
    } // if
    
    cerr << "send_flush_request rpc failed: " << status.error_code() 
    	 << ": " << status.error_message() << std::endl;
    return -1;

} // send_flush_request



