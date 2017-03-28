#include <stdint.h>
#include <stdio.h>

#define GOLAY_POLY  0xAE3

/*
 * golay_codeword_t
 * Type containing two golay [24;12] codewords.
 * In total it represents three bytes of original data.
 * Order (little-endian: least significant byte first):
 *   1st codeword {data_a,checkbits_a,parity_a},
 *   2nd codeword {data_b,checkbits_b,parity_b}
 */
typedef struct {
  uint64_t data_a : 12;
  uint64_t check_a : 11;
  uint64_t parity_a : 1;

  uint64_t data_b : 12;
  uint64_t check_b : 11;
  uint64_t parity_b : 1;

  uint64_t _unused : 16; // here for the compiler to init memory
} golay_codeword_t;


const char* byte_to_binary(uint8_t x)
{
  static char b[9];
  b[0] = '\0';

  for(int i = 0; i < 8; i++) {
    b[i] = (x & (128 >> i)) ? '1' : '0';
  }

  return b;
}

/* bits has to be between 1 and 65
 */
const char* bits_to_binary(uint64_t x, size_t nbits)
{
  static char b[65];

  uint64_t mask = 1L << (nbits-1);
  for(unsigned int i = 0; i < nbits; i++) {
    b[i] = (x & mask) ? '1' : '0';
    mask >>= 1;
  }
  b[nbits] = '\0';

  return b;
}

void print_golay_codeword(golay_codeword_t x)
{
  uint64_t codeword_int = *(uint64_t*)&x;
  uint8_t *codeword_data = (uint8_t*)&x;
  printf(
    "Golay codewords:\n  %ld\n  0x%lx\n  ",
    codeword_int,
    codeword_int
  );

  for(int i=0; i < 6; i++) {
    printf("%s ", bits_to_binary(codeword_data[i], 8));
  }
  printf("\n");
  printf("  data_a\t%s\n", bits_to_binary(x.data_a, 12));
  printf("  check_a\t%s\n", bits_to_binary(x.check_a, 11));
  printf("  parity_a\t%d\n", x.parity_a);
  printf("  data_b\t%s\n", bits_to_binary(x.data_b, 12));
  printf("  check_b\t%s\n", bits_to_binary(x.check_b, 11));
  printf("  parity_b\t%d\n", x.parity_b);
}

uint8_t golay_calc_parity(uint16_t word_x, uint16_t word_y);
uint16_t golay_calc_checkbits(uint16_t codeword);

golay_codeword_t golay_enc(uint8_t byte_a, uint8_t byte_b, uint8_t byte_c)
{
  golay_codeword_t result;

  // bytes to 12-bit words
  result.data_a = byte_a;
  result.data_a |= byte_b << 8; // skips upper 4 bits cause data_a is only 12 bit
  result.data_b = byte_b >> 4;
  result.data_b |= byte_c << 4;

  result.check_a = golay_calc_checkbits(result.data_a);
  result.check_b = golay_calc_checkbits(result.data_b);

  result.parity_a = golay_calc_parity(result.data_a, result.check_a);
  result.parity_b = golay_calc_parity(result.data_b, result.check_b);

  return result;
}

uint16_t golay_calc_checkbits(uint16_t codeword)
{
  for(int i = 1; i <= 12; i++) {
    if(codeword & 1) {
      codeword ^= GOLAY_POLY;
    }
    codeword >>= 1;
  }
  return codeword;
}

uint16_t golay_calc_syndrome(uint16_t databits, uint16_t checkbits)
{
  uint32_t codeword = (checkbits << 12) | databits;
  for(int i = 1; i <= 12; i++) {
    if(codeword & 1) {
      codeword ^= GOLAY_POLY;
    }
    codeword >>= 1;
  }
  return codeword;
}

uint8_t golay_calc_parity(uint16_t word_x, uint16_t word_y)
{
  // xor bytes of the codeword
  uint8_t p;
  p  = *(uint8_t*)&word_x;
  p ^= *((uint8_t*)&word_x + 1);
  p ^= *(uint8_t*)&word_y;
  p ^= *((uint8_t*)&word_y + 1);

  p ^= p >> 4;
  p ^= p >> 2;
  p ^= p >> 1;

  return p & 1;
}


int main()
{
  char* msg = "qerty";
  golay_codeword_t codeword = golay_enc(0x01, 0xff, 0xaa);
  print_golay_codeword(codeword);

  codeword.check_a = golay_calc_syndrome(codeword.data_a, codeword.check_a);

  print_golay_codeword(codeword);

  return 0;
}
