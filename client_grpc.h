#ifndef __CLIENT_GRPC_H__
#define __CLIENT_GRPC_H__

#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <grpc++/grpc++.h>
#include <dirent.h>

#include "nfs.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using nfs::grpcNfs;


using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

struct stat;

class ClientGrpc {
	public:
		ClientGrpc(std::shared_ptr<Channel> channel)
			: stub_(grpcNfs::NewStub(channel)) {}
       
		int send_getattr_request( const char* path, struct stat* stbuf );
		
        int send_readdir_request( const char* path, std::vector<dirent> &dirs );
        
        int send_open_request(const char* path, const int flags, uint64_t &fh );
        
        int send_read_request(const char* path, char *buf, int &error,
                              const size_t size, const off_t offset, uint64_t fh );
                              
        int send_mkdir_request( const char *path, mode_t mode );

        int send_rmdir_request( const char *path );   
        
        int send_create_request( const char *path, mode_t mode, const int flags, uint64_t &fh );
        
        int send_write_request( const char *path, const char *buf, size_t size, off_t offset, uint64_t fh );
        
        int send_unlink_request( const char *path );
        
        int send_truncate_request( const char *path, off_t size, uint64_t fh );
        
        int send_rename_request( const char *from, const char *to );
        
        int send_flush_request( const char *path, uint64_t fh );
	private:
		std::unique_ptr<grpcNfs::Stub> stub_;
};

#endif 

