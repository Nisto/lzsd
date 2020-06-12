#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
  uint8_t *dst, *dict;

  uint16_t i, word, size, flags_bits_remaining,
           num_size_bits, num_flags_bits,
           num_flags_bytes;

  uint32_t flags, offset_mask;

  int32_t todo;

  *aOutSize = todo = get_u32_le(src+0x24);
  *aOutData = dst = malloc(todo);

  if (dst == NULL) {
    printf("ERROR: Could not allocate output buffer for LZS data\n");
    exit(EXIT_FAILURE);
  }

  num_flags_bits = get_u16_le(src+0x28);
  num_flags_bytes = num_flags_bits / 8;
  num_size_bits = get_u16_le(src+0x2A);
  offset_mask = 0xFFFF >> (16 - num_size_bits);
  src += 0x20 + get_u16_le(src+0x22);

  while (todo) {
    for (i = flags = 0; i < num_flags_bytes; i++, src++) {
      flags |= (*src) << (i * 8);
    }

    for (flags_bits_remaining = num_flags_bits; todo && flags_bits_remaining; flags_bits_remaining--, flags >>= 1) {
      if (flags & 1) { /* raw data (uncompressed) */
        *dst++ = *src++;
        todo = todo - 1;
      } else { /* dictionary data (compressed) */
        word = (src[0] << 8) | src[1];
        size = (word & offset_mask) + 3;
        src  = src + 2;
        todo = todo - size;
        dict = dst - (word >> num_size_bits);
        do { *dst++ = *dict++; } while (--size);
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
