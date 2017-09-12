#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <unordered_map>

#include <grpc++/grpc++.h>

#include "nfs.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using nfs::grpcNfs;

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

using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using namespace std;

static string root_path;
static int root_fd;

static char *read_buffer;
static int read_buffer_size;

//inline int inline fd( uint64_t fh ) { return (fh & 0xffffffff); }
//inline uint64_t fh( int fd, int ino ) { return ( ino << 32 ) | fd; }

//unordered_map<uint64_t, string> fh_to_data; // use string as data container

static void write_stat( Stat &dest, const struct stat &src ) {
    dest.set_sta_dev( src.st_dev );
    dest.set_sta_ino( src.st_ino );
    dest.set_sta_mode( src.st_mode );
    dest.set_sta_nlink( src.st_nlink );
    dest.set_sta_uid( src.st_uid );
    dest.set_sta_gid( src.st_gid );
    dest.set_sta_rdev( src.st_rdev );
    dest.set_sta_size( src.st_size );
    dest.set_sta_blksize( src.st_blksize );
    dest.set_sta_blocks( src.st_blocks );
    dest.set_sta_atime( src.st_atime );
    dest.set_sta_mtime( src.st_mtime );
    dest.set_sta_ctime( src.st_ctime );
} // read_stat

// Returns a relative path by removing '/' at the front...
static string get_rel_path( const char *path ) {
    int i = 0;
    while ( path[i] == '/' || isspace( path[i] ) ) {
        i += 1;
    } // while
    
    string ret(path + i);
    //cerr << "get_rel_path: " << path << " returns: " << ret << endl;
    return ret;
} // get_path

static string get_rel_path( const string &path ) {
    return get_rel_path( path.c_str() );
} // get_path


// Returns the absolute path on the server
static string get_path( const string &path ) {
    // is get_rel_path here necessary?
    string ret = root_path + get_rel_path( path );
    //cerr << "get_path: " << path << " returns: " << ret << endl;
    return ret;
} // get_path



// In order to pass errno, always return Status::OK...

class NfsServiceImpl final : public grpcNfs::Service {
  
  Status getattr_request(ServerContext* context, const GetattrRequest* request, 
      GetattrReply* reply) override {
    
    struct stat stbuf;
    
    string rel_path = get_rel_path(request->path());
    
    // fstatat doesn't work here, don't know why...
    
    // local file system call
    
    //int res = fstatat( root_fd, 
    //                   rel_path.c_str(), 
    //                   &stbuf,
    //                   AT_NO_AUTOMOUNT ); 
    
    string path = get_path( request->path() );
    int res = lstat( path.c_str(), &stbuf );
    //cerr << "getattr: root_fd = " << root_fd << ", path = " << path << endl;
    //cerr << "res = " << res << endl;
    Stat* attr = reply->mutable_attr();
    
    // copy statbuf to attr
    write_stat( *attr, stbuf );
    
    if ( -1 == res ) {        
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
        return Status::OK;        
    } // if
    reply->set_err( 0 );
    return Status::OK;
  } // getattr_request
  
  
  
  Status readdir_request(ServerContext* context, const ReaddirRequest* request, 
      ReaddirReply* reply) override {
    
    string path = get_path(request->path());
    DIR *dp = opendir( path.c_str() );
    if ( NULL == dp ) {
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
        return Status::OK;
    } // if
    
    dirent *de;
    
    while ( (de = readdir( dp )) != NULL ) {
        Dirent *De = reply->add_de();
        De->set_d_ino( de->d_ino );
        De->set_d_type( de->d_type );
        De->set_d_name( de->d_name );              
    } // while
    
    
    reply->set_err( 0 ); 
    return Status::OK;
  } // readdir_request
  
  
  Status open_request(ServerContext* context, const OpenRequest* request, 
      OpenReply* reply) override {
    
    string rel_path = get_rel_path(request->path());
    int res = openat( root_fd, 
                      rel_path.c_str(), 
                      request->flags() );
    if ( -1 == res ) {
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
        return Status::OK;
    }
    
    //fh_to_data[res] = "";
    
    //cerr << "open: " << rel_path << ", fd = " << res << endl;
    reply->set_fh( res );
    reply->set_err( 0 );
    return Status::OK;
  } // open_request
  
  
  Status read_request(ServerContext* context, const ReadRequest* request, 
      ReadReply* reply) override {
    
    // buffer is too short; make it larger
    if ( request->size() > read_buffer_size ) {
        read_buffer_size = request->size() + 1;
        delete[] read_buffer;
        read_buffer = new char[read_buffer_size];
    } // if
    
    //cerr << "read: " << request->path() << ", fd = " << request->fh() << endl;
    int res = pread( request->fh(), read_buffer, request->size(), request->offset() );
    
    if ( -1 == res ) {
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
        return Status::OK;
    } // if
    
    reply->set_data( read_buffer, res );
    
    return Status::OK;
  } // read_request
  
  
  Status mkdir_request(ServerContext* context, const MkdirRequest* request, 
      MkdirReply* reply) override {
    
    string rel_path = get_rel_path( request->path() );
    
    int res = mkdirat( root_fd, rel_path.c_str(), request->mode() );
    
    if ( -1 == res ) {
        // failure
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
    } else {
        // success
        reply->set_err( 0 );
    } // if
            
    return Status::OK;
  } // mkdir_request
  
  
  Status rmdir_request(ServerContext* context, const RmdirRequest* request, 
      RmdirReply* reply) override {
    
    string path = get_path( request->path() );
    
    int res = rmdir( path.c_str() );
    
    if ( -1 == res ) {
        // failure
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
    } else {
        // success
        reply->set_err( 0 );
    } // if
            
    return Status::OK;
  } // rmdir_request
  
  Status create_request(ServerContext* context, const CreateRequest* request, 
      CreateReply* reply) override {
    
    string rel_path = get_rel_path(request->path());
    int res = openat( root_fd, 
                      rel_path.c_str(), 
                      request->flags(),
                      request->mode() );
                      
    if ( -1 == res ) {
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
        return Status::OK;
    }
    
    reply->set_fh( res );
    reply->set_err( 0 );
    return Status::OK;
  } // create_request
  
  Status write_request(ServerContext* context, const WriteRequest* request, 
      WriteReply* reply) override {
    
    //cerr << "write: " << request->path() << " , fd=" << request->fh() << endl;
    int res = pwrite( request->fh(), 
                      request->data().c_str(), 
                      request->data().size(),
                      request->offset() ); 
    
    if ( -1 == res ) {
        reply->set_err( -errno );
        reply->set_len( - 1);
        return Status::OK;
    } //if
    
    reply->set_err( 0 );
    reply->set_len( res );
    return Status::OK;
  } // create_request
  
  Status unlink_request(ServerContext* context, const UnlinkRequest* request, 
      UnlinkReply* reply) override {
    
    string path = get_rel_path( request->path() );
    
    int res = unlinkat( root_fd, path.c_str(), 0 );
    
    if ( -1 == res ) {
        // failure
        reply->set_err( -errno );
       // cerr << "-errno = " << reply->err() << endl;
    } else {
        // success
        reply->set_err( 0 );
    } // if
            
    return Status::OK;
  } // rmdir_request
  
  Status truncate_request(ServerContext* context, const TruncateRequest* request, 
      TruncateReply* reply) override {
    
    int res = ftruncate( request->fh(), request->size() );
    
    if ( -1 == res ) {
        // failure
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
    } else {
        // success
        reply->set_err( 0 );
    } // if
            
    return Status::OK;
  } // rmdir_request
  
  Status rename_request(ServerContext* context, const RenameRequest* request, 
      RenameReply* reply) override {
    
    string from = get_path( request->from_name() );
    string to = get_path( request->to_name() );
    
    int res = rename( from.c_str(), to.c_str() );
    
    if ( -1 == res ) {
        // failure
        reply->set_err( -errno );
        //cerr << "-errno = " << reply->err() << endl;
    } else {
        // success
        reply->set_err( 0 );
    } // if
            
    return Status::OK;
  } // rmdir_request
  
  
  Status flush_request(ServerContext* context, const FlushRequest* request, 
      FlushReply* reply) override {
    //cerr << "flush: " << request->fh() << endl;
    int res = fsync( request->fh() );
    
    if ( -1 == res ) {
        reply->set_err( - errno );
    } else {
        reply->set_err( 0 );
    } // if
    
    
    // actually this is called upon release() ... 
    close( request->fh() );
            
    return Status::OK;
  } // rmdir_request
  
      
}; // NfsServiceImpl
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
void RunServer() {
  //change the IP address to test on different machine
  string server_port;
  string server_host;
  parse_config(server_port, server_host, "server.config");
  string server_address = server_host+":"+server_port;
  NfsServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
} // RunServer

int main(int argc, char** argv) {
  DIR *root_dir = opendir( argv[1] );
  
  if ( NULL == root_dir ) {
     cerr << "Failed to open given directory: " << argv[1] << endl;
     return 1;
  } // if
  
  root_fd = dirfd( root_dir );
  
  if ( 0 > root_fd ) {
     cerr << "Failed to open given directory: " << argv[1] << endl;
     return 1;
  } // if
  
  read_buffer_size = 4096;
  read_buffer = new char[read_buffer_size];
  
  root_path = string( argv[1] ) + '/'; 
  
  RunServer();

  close( root_fd );
  return 0;
} // main




