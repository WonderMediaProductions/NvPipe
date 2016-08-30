/*
 * Copyright (c) 2016 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual
 * property and proprietary rights in and to this software,
 * related documentation and any modifications thereto.  Any use,
 * reproduction, disclosure or distribution of this software and
 * related documentation without an express license agreement from
 * NVIDIA CORPORATION is strictly prohibited.
 *
 */
#include <stdio.h>
#include <inttypes.h>
#include "nvpipe.h"
#include "libnvpipeutil/format.h"

void SaveBufferRGBA(uint8_t *data, int width, int height, char *str) {
  FILE *pFile;
  
  // Open file
  pFile=fopen(str, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  uint8_t *row = malloc( sizeof(uint8_t) * width * 3 );

  // Write pixel data
  for(int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      int index = x + y*width;
      row[x*3] = data[index*4];
      row[x*3+1] = data[index*4+1];
      row[x*3+2] = data[index*4+2];
    }
    fwrite(row, 1, width*3, pFile);
  }
  
  free (row);
  // Close file
  fclose(pFile);
}

void SaveBufferBit(uint8_t *data, int length, char *str) {
  FILE *pFile;

  // Open file
  pFile=fopen(str, "ab");
  if(pFile==NULL)
    return;

  fwrite(data, 1, length, pFile);
  
  // Close file
  fclose(pFile);
}

int main( int argc, char* argv[] ) {

    nvpipe* codec_1 = nvpipe_create_instance(NVPIPE_CODEC_ID_H264_HARDWARE);
    //nvpipe* codec_1 = nvpipe_create_instance(NVPIPE_CODEC_ID_H264_SOFTWARE);
    nvpipe* codec_2 = nvpipe_create_instance(NVPIPE_CODEC_ID_H264_HARDWARE);

    int width = 1980;
    int height = 1080;

    size_t buffer_size = sizeof(uint8_t)*width*height*4;
    void* img_buffer = malloc(buffer_size);
    size_t img_buffer_size = buffer_size;
    uint8_t* img_ptr0 = img_buffer;
    void* pkt_buffer = malloc(buffer_size);
    size_t pkt_buffer_size = buffer_size;

    int channel = 4;
    nvpipeMemGpu2 memgpu2_;
    initializeMemGpu2(&memgpu2_);

    for (int i = 0; i < 10; i++ ) {
        pkt_buffer_size = buffer_size;
        for(size_t y=0;y<height;y++) {
            for(size_t x=0;x<width;x++) {
                int index = y * width + x;
                if ( channel == 4) {
                    img_ptr0[index*channel] = x+y+i*5;//x+y;
                    img_ptr0[index*channel+1] = x+i*10;//x;
                    img_ptr0[index*channel+2] = 0;
                    img_ptr0[index*channel+3] = 255;
                } else {
                    img_ptr0[index*channel] = x+y;
                    img_ptr0[index*channel+1] = x;
                    img_ptr0[index*channel+2] = 0;
                }
            }
        }

        char image_filename[20];
        sprintf(image_filename, "original_%d.pgm", i);
        SaveBufferRGBA(img_buffer, width, height, image_filename);

        if ( nvpipe_encode(codec_1, img_buffer, buffer_size, pkt_buffer, &pkt_buffer_size, width, height, NVPIPE_IMAGE_FORMAT_RGBA) ) {
            printf("encoding size: %zu\n", pkt_buffer_size);
            SaveBufferBit(pkt_buffer, pkt_buffer_size, "file.264");

            if ( nvpipe_decode(codec_2, pkt_buffer, pkt_buffer_size, img_buffer, img_buffer_size, &width, &height, NVPIPE_IMAGE_FORMAT_RGBA) ) {
                sprintf(image_filename, "decoded_%d.pgm", i);
                SaveBufferRGBA(img_buffer, width, height, image_filename);
            }
        }
    }

    destroyMemGpu2(&memgpu2_);
    nvpipe_destroy_instance(codec_1);
    nvpipe_destroy_instance(codec_2);
    free(img_buffer);
    free(pkt_buffer);
    return 0;

}
