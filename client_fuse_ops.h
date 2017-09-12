#ifndef __CLIENT_FUSE_OPS_H__
#define __CLIENT_FUSE_OPS_H__

#define FUSE_USE_VERSION 30

#include "config.h"

#include <fuse.h>
#include <unordered_map>


void *nfs_init(struct fuse_conn_info *conn,
			   struct fuse_config *cfg);
    
	
int nfs_getattr(const char *path, struct stat *stbuf,
			    struct fuse_file_info *fi);
			    
int nfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags);
			
int nfs_open(const char *path, struct fuse_file_info *fi);

int nfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi);


#endif // __CLIENT_FUSE_OPS_H__
