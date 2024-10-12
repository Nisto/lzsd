#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// "Gintama - Yorozuya Chuubu - Tsukkomable Douga" (Wii) LZS decompressor

uint16_t get_u16_be(uint8_t *mem)
{
  return (mem[0] << 8) | mem[1];
}

uint32_t get_u32_be(uint8_t *mem)
{
  return (mem[0] << 24) | (mem[1] << 16) | (mem[2] << 8) | mem[3];
}

void decompress(uint8_t *src, uint8_t **aOutData, long *aOutSize)
{
  uint8_t *dst, *dict, *comd, *rawd, threshold;

  uint16_t word, size, flags_bits_remaining, num_size_bits, size_mask;

  uint32_t flags;

  int32_t todo;

  // uint8_t bits_per_unit = src[0x06]; // 8/16/32

  comd = src + get_u32_be(src+0x08);
  rawd = src + get_u32_be(src+0x0C);

  *aOutSize = todo = get_u32_be(src+0x10);
  *aOutData = dst = malloc(todo);

  if (dst == NULL) {
    printf("ERROR: Could not allocate output buffer for LZS data\n");
    exit(EXIT_FAILURE);
  }

  num_size_bits = src[0x05];
  threshold = src[0x19];

  size_mask = 0xFFFF >> (16 - num_size_bits);

  while (todo) {
    for (flags=get_u16_be(comd), comd=comd+2, flags_bits_remaining=16; todo && flags_bits_remaining; flags_bits_remaining--, flags>>=1) {
      if (flags & 1) { /* raw data (uncompressed) */
        *dst++ = *rawd++;
        todo = todo - 1;
      } else { /* dictionary data (compressed) */
        word = (comd[0] << 8) | comd[1];
        comd = comd + 2;
        size = (word & size_mask) + threshold;
        todo = todo - size;
        dict = dst - (word >> num_size_bits);
        do {
          *dst++ = *dict++;
        } while (--size);
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
    printf("Gintama - Yorozuya Chuubu - Tsukkomable Douga (Wii) LZS decompressor\n");
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
