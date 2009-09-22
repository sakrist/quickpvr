/*******************************************************************************
  Copyright (c) 2009, Limbic Software, Inc.
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the Limbic Software, Inc. nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY LIMBIC SOFTWARE, INC. ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL LIMBIC SOFTWARE, INC. BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/
#include "pvr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct PVRHeader
{
    uint32_t      size;
    uint32_t      height;
    uint32_t      width;
    uint32_t      mipcount;
    uint32_t      flags;
    uint32_t      texdatasize;
    uint32_t      bpp;
    uint32_t      rmask;
    uint32_t      gmask;
    uint32_t      bmask;
    uint32_t      amask;
    uint32_t      magic;
    uint32_t      numtex;
} PVRHeader;

PVRTexture::PVRTexture()
:data(NULL)
{
}

PVRTexture::~PVRTexture()
{
    if(this->data)
        free(this->data);
}

ePVRLoadResult PVRTexture::load(const char *const path)
{
    uint8_t *data;
    unsigned int length;

    FILE *fp = fopen(path, "rb");
    if(fp==NULL)
        return PVR_LOAD_FILE_NOT_FOUND;

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data = (uint8_t*)malloc(length);
    fread(data, 1, length, fp);

    fclose(fp);

    if(length<sizeof(PVRHeader))
    {
        free(data);
        return PVR_LOAD_INVALID_FILE;
    }

    // parse the header
    uint8_t* p = data;
    PVRHeader *header = (PVRHeader *)p;
    p += sizeof( PVRHeader );

    if( header->size != sizeof( PVRHeader ) )
    {
        free( data );
        return PVR_LOAD_INVALID_FILE;
    }

    if( header->magic != 0x21525650 )
    {
        free( data );
        return PVR_LOAD_INVALID_FILE;
    }

    if( header->numtex != 1 )
    {
        free( data );
        return PVR_LOAD_MORE_THAN_ONE_SURFACE;
    }

    int ptype = header->flags & PVR_PIXELTYPE_MASK;
    printf("Pixeltype: %i\n", ptype);

    this->width = header->width;
    this->height = header->height;

    printf("Width: %i\n", this->width);
    printf("Height: %i\n", this->height);

    this->data = (uint8_t*)malloc(this->width*this->height*4);

    switch(ptype)
    {
    case PVR_TYPE_RGBA4444:
        {
            uint8_t *in  = p;
            uint8_t *out = this->data;
            for(int y=0; y<this->height; ++y)
            for(int x=0; x<this->width; ++x)
            {
                int v1 = *in++;
                int v2 = *in++;

                uint8_t a = (v1&0x0f)<<4;
                uint8_t b = (v1&0xf0);
                uint8_t g = (v2&0x0f)<<4;
                uint8_t r = (v2&0xf0);

                *out++ = r;
                *out++ = g;
                *out++ = b;
                *out++ = a;
            }
        }
        break;
    case PVR_TYPE_AI8:
        {
            uint8_t *in  = p;
            uint8_t *out = this->data;
            for(int y=0; y<this->height; ++y)
            for(int x=0; x<this->width; ++x)
            {
                int a = *in++;
                int i = *in++;

                *out++ = i;
                *out++ = i;
                *out++ = i;
                *out++ = a;
            }
        }
        break;
    default:
        printf("unknown PVR type %i!\n", ptype);
        free(this->data);
        free(data);
        return PVR_LOAD_UNKNOWN_TYPE;
    }

    free(data);
    return PVR_LOAD_OKAY;

//
//    int compressed = 0;
//    int format;
//    int type;
//    int alpha = header->amask>0;
//    int size = header->width*header->height*header->bpp/8;
//
//    switch( ptype )
//    {
//    case PVR_TYPE_RGBA4444:
//        format = GL_UNSIGNED_SHORT_4_4_4_4;
//        type = GL_RGBA;
//        break;
//    case PVR_TYPE_RGBA5551:
//        format = GL_UNSIGNED_SHORT_5_5_5_1;
//        type = GL_RGBA;
//        break;
//    case PVR_TYPE_RGBA8888:
//        format = GL_UNSIGNED_BYTE;
//        type = GL_RGBA;
//        break;
//    case PVR_TYPE_RGB565:
//        format = GL_UNSIGNED_SHORT_5_6_5;
//        type = GL_RGB;
//        break;
//    case PVR_TYPE_RGB888:
//        format = GL_UNSIGNED_BYTE;
//        type = GL_RGB;
//        break;
//    case PVR_TYPE_I8:
//        format = GL_UNSIGNED_BYTE;
//        type = GL_LUMINANCE;
//        break;
//    case PVR_TYPE_AI8:
//        format = GL_UNSIGNED_BYTE;
//        type = GL_LUMINANCE_ALPHA;
//        //printf( "ai8 %dx%d, %d\n", header->height, header->width, size );
//        break;
//    case PVR_TYPE_PVRTC2:
//        format = alpha?GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
//        type = alpha?GL_RGBA:GL_RGB;
//        compressed = 1;
//        size = MAX( header->width*header->height*header->bpp/8, 32 );
//        //printf( "PVR 2bpp alpha %d %dx%d, %d\n", alpha, header->height, header->width, size );
//        break;
//    case PVR_TYPE_PVRTC4:
//        format = alpha?GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
//        type = alpha?GL_RGBA:GL_RGB;
//        compressed = 1;
//        size = MAX( header->width*header->height*header->bpp/8, 32 );
//        //printf( "PVR 4bpp alpha %d %dx%d, %d\n", alpha, header->height, header->width, size );
//        break;
//    default:
//        printf( "Failed to load .pvr file: unknown format 0x%02x!\n", ptype );
//        free( data );
//        return false;
//    }
//
//    if( compressed == 1 )
//    {
//        if( header->height != header->width )
//            printf( "Problem loading .pvr file: not a square texture!\n" );
//    }
//
//    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE );
//
//    int totalread = 0;
//
//    for( int i=0; i<=header->mipcount; ++i )
//    {
//        if( compressed )
//        {
//            int w = header->width>>i;
//            int fsize = MAX( w*w*header->bpp/8, 32 );
//            glCompressedTexImage2D( GL_TEXTURE_2D, i, format, w, w, 0, fsize, p
//                    );
//            p += fsize;
//            textureMemory += fsize;
//            compressedTextureMemory += fsize;
//            totalread += fsize;
//        } else
//        {
//            int w = header->width>>i;
//            int h = header->height>>i;
//            int fsize = (w*h*header->bpp+7)/8;
//            glTexImage2D( GL_TEXTURE_2D, i, type, w, h, 0, type, format, p );
//            p += fsize;
//            textureMemory += fsize;
//            uncompressedTextureMemory += fsize;
//            totalread += fsize;
//        }
//    }
//
//    free( data );
//
//    /** Removed this check, because it seems the exporter writes a bogus
//     * texdatasize into the header-> */
//    //if( totalread != header->texdatasize )
//    //    printf( "Warning: loading PVR file '%s', loaded tex size (%u) does not "
//    //            "match size in header (%u), compressed %i\n", name, totalread,
//    //            header->texdatasize, compressed );
//
//    return true;
}
