project (OpenCTM)

# OpenCTM comes with lzma
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/liblzma)

add_library(OpenCTM STATIC
	liblzma/Alloc.c
 	liblzma/LzFind.c
 	liblzma/LzmaDec.c
 	liblzma/LzmaEnc.c
 	liblzma/LzmaLib.c

	compressMG1.c
 	compressMG2.c
 	compressRAW.c
 	openctm.c
 	stream.c
    openctmpp.cpp
 	
 	internal.h
 	openctm.h
 	openctmpp.h
)
