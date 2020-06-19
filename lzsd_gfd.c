#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define min(a,b) (((a)<(b))?(a):(b))

// Gear Fighter Dendoh (PS1) LZS decompressor

uint16_t get_u16_le(uint8_t *mem)
{
  return (mem[1] << 8) | mem[0];
}

uint32_t get_u32_le(uint8_t *mem)
{
  return (mem[3] << 24) | (mem[2] << 16) | (mem[1] << 8) | mem[0];
}

void decompress(uint8_t *src, uint8_t **aOutData, long *aOutSize)
{
  uint8_t *dst, *dict, num_offset_bits_high, offset_high_mask, size;

  uint16_t flags, flags_mask;

  uint32_t version;

  int32_t todo;

  version = get_u32_le(src+0x00);
  *aOutSize = todo = get_u32_le(src+0x04);
  num_offset_bits_high = get_u32_le(src+0x08) - 8;
  offset_high_mask = (1 << num_offset_bits_high) - 1;

  if (version == 1)
    src += 0x10;
  else if (version == 3)
    src += 0x1C;
  else
    return;

  *aOutData = dst = malloc(todo);

  if (dst == NULL) {
    printf("ERROR: Could not allocate output buffer for LZS data\n");
    exit(EXIT_FAILURE);
  }

  while (todo) {
    flags = (src[1] << 8) | src[0];
    src = src + 2;
    if (flags == 0) {
      size = min(todo, 16);
      todo = todo - size;
      do { *dst++ = *src++; } while (--size);
    } else {
      for (flags_mask = 1; todo && (flags_mask & 0xFFFF); flags_mask <<= 1) {
        if (flags & flags_mask) {
          dict = dst - ((((src[1] & offset_high_mask) << 8) | src[0]) + 1);
          size = (src[1] >> num_offset_bits_high) + 3;
          src  = src + 2;
          todo = todo - size;
          do { *dst++ = *dict++; } while (--size);
        } else {
          *dst++ = *src++;
          todo = todo - 1;
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  FILE *f = NULL;
  long insize = 0;
  long outsize = 0;
  char *inpath = NULL;
  char *outpath = NULL;
  uint8_t *indata = NULL;
  uint8_t *outdata = NULL;

  //////////////////////////////////////////////////////////////////

  if (argc != 2 && argc != 3) {
    printf("Gear Fighter Dendoh LZS decompressor\n");
    printf("Usage: %s <infile> [outfile]\n", argv[0]);
    return 1;
  }

  //////////////////////////////////////////////////////////////////

  inpath = argv[1];

  f = fopen(inpath, "rb");
  if (f == NULL) {
    printf("ERROR: Could not open input file\n");
    return 1;
  }

  fseek(f, 0, SEEK_END);
  insize = ftell(f);
  fseek(f, 0, SEEK_SET);

  indata = malloc(insize);
  if (indata == NULL) {
    printf("ERROR: Could not allocate buffer for input data\n");
    return 1;
  }

  if (fread(indata, 1, insize, f) != insize) {
    printf("ERROR: Could not read input data!\n");
    return 1;
  }

  fclose(f);

  //////////////////////////////////////////////////////////////////

  decompress(indata, &outdata, &outsize);

  free(indata);

  //////////////////////////////////////////////////////////////////

  if (argc < 3) {
    outpath = malloc(strlen(inpath) + 1 + 1);
    sprintf(outpath, "%sD", inpath);
  } else {
    outpath = argv[2];
  }

  f = fopen(outpath, "wb");
  if (f == NULL) {
    printf("ERROR: Could not open output file\n");
    return 1;
  }

  if (fwrite(outdata, 1, outsize, f) != outsize) {
    printf("ERROR: Could not write output data!\n");
    return 1;
  }

  fclose(f);

  //////////////////////////////////////////////////////////////////

  free(outdata);

  if (argc < 3)
    free(outpath);

  return 0;
}
