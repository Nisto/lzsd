#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Natsume (NDS) LZS decompressor

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
  const uint8_t num_size_bits = src[0x05];
  const uint8_t bits_per_unit = src[0x06]; // 8 / 16 / 32
  const uint8_t min_word_size = src[0x19];

  uint8_t *comd = src + get_u32_le(src+0x08);
  uint8_t *rawd = src + get_u32_le(src+0x0C);

  uint8_t *dst = NULL;

  const uint16_t size_mask = 0xFFFF >> (16 - num_size_bits);

  const uint32_t out_size = get_u32_le(src+0x10);

  uint32_t units;

  if (bits_per_unit != 8 && bits_per_unit != 16 && bits_per_unit != 32) {
    printf("ERROR: Unexpected bits_per_unit: %d\n", bits_per_unit);
    exit(EXIT_FAILURE);
  }

  dst = malloc(out_size);

  if (dst == NULL) {
    printf("ERROR: Could not allocate output buffer for LZS data\n");
    exit(EXIT_FAILURE);
  }

  *aOutData = dst;
  *aOutSize = out_size;

  units = out_size / (bits_per_unit / 8);

  while (units) {
    uint16_t flags_bits_remaining = 16;
    uint16_t flags = get_u16_le(comd);

    comd += 2;

    for (; units && flags_bits_remaining; flags_bits_remaining--, flags>>=1) {
      if (flags & 1) {
        // Raw data
        if (bits_per_unit == 8) {
          *dst++ = *rawd++;
        } else if (bits_per_unit == 16) {
          *(uint16_t *)dst = *(uint16_t *)rawd;
          dst += 2;
          rawd += 2;
        } else {
          *(uint32_t *)dst = *(uint32_t *)rawd;
          dst += 4;
          rawd += 4;
        }
        units--;
      } else {
        // Dictionary copy
        uint16_t word = get_u16_le(comd);
        uint32_t size = (word & size_mask) + min_word_size;
        uint32_t offset = word >> num_size_bits;

        comd += 2;
        units -= size;

        if (bits_per_unit == 8) {
          uint8_t *dict = dst - offset;
          while (size--) *dst++ = *dict++;
        } else if (bits_per_unit == 16) {
          uint16_t *dict = (uint16_t *)(dst - offset * 2);
          while (size--) {
            *(uint16_t *)dst = *dict++;
            dst += 2;
          }
        } else {
          uint32_t *dict = (uint32_t *)(dst - offset * 4);
          while (size--) {
            *(uint32_t *)dst = *dict++;
            dst += 4;
          }
        }
      }
    }
  }

  if (src[0x18]) {
    memcpy(dst, src+0x1C, src[0x18]);
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
    printf("Natsume (NDS) LZS decompressor\n");
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
