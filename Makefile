CXX = g++
CXXFLAGS = -std=c++11 -g -MMD -Wall `pkg-config fuse3 --cflags --libs`
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PROTOS_PATH = ./
# OBJECTS1 = client_nfs.o client_fuse_ops.o client_sock.o
# OBJECTS2 = server_nfs.o
# OBJECTS = $(OBJECTS1) $(OBJECTS2) 

# DEPENDS = ${OBJECTS:.o=.d}

vpath %.proto $(PROTOS_PATH)

all: client_grpc server_grpc

client_grpc: nfs.pb.o nfs.grpc.pb.o client_grpc.o client_fuse_ops.o
	$(CXX) $^ $(LDFLAGS) $(CXXFLAGS) -o $@

server_grpc: nfs.pb.o nfs.grpc.pb.o server_grpc.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h *.d client_grpc server_grpc
