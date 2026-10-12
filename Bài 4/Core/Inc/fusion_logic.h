#ifndef FUSION_LOGIC_H
#define FUSION_LOGIC_H
#include <stdint.h>

typedef struct {
  uint32_t frame;
  uint8_t bits;
  uint8_t active;
} NEC_Decoder;

typedef struct {
  uint8_t mode, step, led4;
  uint8_t display, hex_display;
  uint8_t learning, learn_index, ready;
  uint8_t keys[5]; /* OK, 1, 2, 3, 4. */
  uint8_t address;
} FusionState;

int NEC_FeedInterval(NEC_Decoder *decoder, uint32_t ticks, uint8_t bytes[4]);
void Fusion_Init(FusionState *state);
void Fusion_BeginLearning(FusionState *state);
/* Return 1 for a learned key, 2 when learning completes, 0 otherwise. */
int Fusion_Command(FusionState *state, const uint8_t bytes[4]);
void Fusion_ButtonIncrement(FusionState *state);
uint8_t Fusion_NextLED(FusionState *state);
uint8_t Fusion_Digit(const FusionState *state, uint8_t units);
#endif
