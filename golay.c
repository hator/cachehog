#include <stdint.h>
#include <stdio.h>

#define GOLAY_POLY  0xAE3

typedef uint8_t bool;

/*
 * golay_codeword_t
 * Type containing two golay [24;12] codewords.
 * In total it represents three bytes of original data.
 * Order (little-endian: least significant byte first):
 *   data, checkbits, parity
 */
typedef struct {
  uint32_t data : 12;
  uint32_t check : 11;
  uint32_t parity : 1;

  uint32_t _unused : 8; // here for the compiler to init memory
} golay_codeword_t;

uint32_t golay_to_int(golay_codeword_t codeword)
{
  return (codeword.check << 12) | codeword.data;
}

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
  uint32_t codeword_int = *(uint32_t*)&x;
  uint8_t *codeword_data = (uint8_t*)&x;
  printf(
    "Golay codewords:\n  %d\n  0x%x\n  ",
    codeword_int,
    codeword_int
  );

  for(int i=0; i < 3; i++) {
    printf("%s ", bits_to_binary(codeword_data[i], 8));
  }
  printf("\n");
  printf("  data\t%s\n", bits_to_binary(x.data, 12));
  printf("  check\t%s\n", bits_to_binary(x.check, 11));
  printf("  parity\t%d\n", x.parity);
}

uint8_t golay_calc_parity(golay_codeword_t codeword);
uint16_t golay_calc_checkbits(uint16_t codeword);

golay_codeword_t golay_encode(uint16_t data)
{
  golay_codeword_t result;

  result.data = data;
  result.check = golay_calc_checkbits(result.data);
  result.parity = golay_calc_parity(result);

  return result;
}

uint16_t golay_calc_checkbits(uint16_t codeword)
{
  for(int i = 0; i < 12; i++) {
    if(codeword & 1) {
      codeword ^= GOLAY_POLY;
    }
    codeword >>= 1;
  }
  return codeword;
}

golay_codeword_t golay_correct(golay_codeword_t codeword);

/*
 * golay_decode
 * NOTE: current implementation only allows for correcting 3 bit errors
 * and detecting 4 bit errors. This one is based on:
 * http://aqdi.com/articles/using-the-golay-error-detection-and-correction-code-3/
 * TODO: use better decoding algorithm to make full use of Golay code
 */
uint16_t golay_decode(golay_codeword_t codeword)
{
  golay_codeword_t corrected = golay_correct(codeword);

  bool parity = golay_calc_parity(corrected);

  print_golay_codeword(corrected);

  // TODO if (parit != codeword.parity)) --> error detected
  return corrected.data;
}

uint32_t golay_calc_syndrome(uint32_t codeword);
int golay_calc_weight(uint32_t codeword);
uint32_t golay_rotate_right23(uint32_t data, size_t nbits);
uint32_t golay_rotate_left23(uint32_t data, size_t nbits);

golay_codeword_t golay_correct(golay_codeword_t codeword_)
{
  golay_codeword_t result;
  uint32_t codeword = golay_to_int(codeword_);
  const uint32_t codeword_save = codeword;
  uint32_t codeword_tmp = codeword;

  uint32_t syndrome;

  int trial_bit, threshold, weight, i;

  for(i=0; i<24; i++) {
    trial_bit = -1;
    threshold = 3;
    codeword_tmp = codeword;

    while(trial_bit < 24) {

      syndrome = golay_calc_syndrome(codeword);
      printf("%s\n", bits_to_binary(syndrome, 23));
      weight = golay_calc_weight(syndrome);
      if(weight <= threshold) {
        codeword ^= syndrome;
        goto exit;
      }

      trial_bit++;
      codeword = codeword_tmp ^ (1 << trial_bit); // try new bit
      threshold = 2;
    }

    codeword = golay_rotate_left23(codeword, 1);
  }
  /*
  trial_bit = -1;
  threshold = 3;
  weight = 0;
  int errors = 0;
  int mask = 1;

  while(trial_bit < 24) {
    if(trial_bit >= 0) {
      codeword = codeword_save ^ (1 << trial_bit);
      threshold = 2;
    }

    syndrome = golay_calc_syndrome(codeword);
    if(!syndrome) {
      printf("No errors");
      // no errors
      goto exit;
    } else {
      for(int i=0; i<23; i++) {
        //printf("%d\n", i);
        errors = golay_calc_weight(syndrome);

        printf("%d\n", errors);
        printf("%s\n", bits_to_binary(syndrome, 23));
        printf("%s\n", bits_to_binary(codeword, 23));
        if(errors <= threshold) {
          printf("Threshold\n");
          codeword ^= syndrome;
          codeword = golay_rotate_right23(codeword, i);
          goto exit;
        } else {
          codeword = golay_rotate_left23(codeword, 1);
          syndrome = golay_calc_syndrome(codeword);
        }
      }
      trial_bit++;
    }
  }// */

  codeword = codeword_save;

exit:
  codeword = golay_rotate_right23(codeword, i);
  result.data = codeword;
  result.check = codeword >> 12;
  return result;
}

uint32_t golay_calc_syndrome(uint32_t codeword)
{
  for(int i = 0; i < 12; i++) {
    if(codeword & 1) {
      codeword ^= GOLAY_POLY;
    }
    codeword >>= 1;
  }
  return codeword << 12;
}

int golay_calc_weight(uint32_t codeword)
{
  const char nibble_weights[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};

  int weight = 0;
  for(int i = 0; i < 6; i++) {
    weight += nibble_weights[codeword & 0xf];
    codeword >>= 4;
  }
  return weight;
}

uint32_t golay_rotate_right23(uint32_t x, size_t nbits)
{
  return ((x >> nbits) | (x << (23 - nbits))) & 0x7ffffff;
}

uint32_t golay_rotate_left23(uint32_t x, size_t nbits)
{
  return ((x << nbits) | (x >> (23 - nbits))) & 0x7ffffff;
}

uint8_t golay_calc_parity(golay_codeword_t codeword)
{
#if defined(__GNUC__)

  return __builtin_parity(golay_to_int(codeword));

#else

  // xor bytes of the codeword
  uint8_t p;
  p  = *(uint8_t*)&codeword.check;
  p ^= *((uint8_t*)&codeword.check + 1);
  p ^= *(uint8_t*)&codeword.data;
  p ^= *((uint8_t*)&codeword.data + 1);

  p ^= p >> 4;
  p ^= p >> 2;
  p ^= p >> 1;

  return p & 1;

#endif /* defined(__GNUC__) */
}


int main()
{
  golay_codeword_t codeword = golay_encode(0x01ff);
  print_golay_codeword(codeword);

  codeword.data &= ~(1 << 3);
  codeword.data &= ~(1 << 6);
  codeword.check |= 1 << 0;

  printf("%ld\n", sizeof(golay_codeword_t));


  print_golay_codeword(codeword);


  uint16_t result = golay_decode(codeword);

  printf("%x\n", result);

  return 0;
}
