#include "fusion_logic.h"
int NEC_FeedInterval(NEC_Decoder *decoder, uint32_t ticks, uint8_t bytes[4])
{
  /* Falling edge to falling edge, in TIM6 ticks of 100 us.
     Leader: 13.5 ms; zero: 1.125 ms; one: 2.25 ms.
     NEC repeat leader (11.25 ms) is deliberately ignored: holding OK
     must not repeatedly toggle LED4. */
  if (ticks >= 125U && ticks <= 145U) {
    decoder->frame = 0;
    decoder->bits = 0;
    decoder->active = 1;
    return 0;
  }
  if (!decoder->active) return 0;
  if (ticks >= 18U && ticks <= 27U)
    decoder->frame |= 1UL << decoder->bits;
  else if (!(ticks >= 8U && ticks <= 15U)) {
    decoder->active = 0;
    return 0;
  }
  if (++decoder->bits == 32U) {
    decoder->active = 0;
    for (unsigned i = 0; i < 4; ++i)
      bytes[i] = (uint8_t)(decoder->frame >> (8U * i));
    return ((bytes[0] ^ bytes[1]) == 0xFFU &&
            (bytes[2] ^ bytes[3]) == 0xFFU);
  }
  return 0;
}

void Fusion_Init(FusionState *state)
{
  state->mode = 1; state->step = 0; state->led4 = 0;
  state->display = 25; state->hex_display = 0;
  state->learning = 0; state->learn_index = 0; state->ready = 0;
  state->address = 0;
  for (unsigned i = 0; i < 5; ++i) state->keys[i] = 0;
}

void Fusion_BeginLearning(FusionState *state)
{
  state->learning = 1; state->learn_index = 0; state->ready = 0;
}

int Fusion_Command(FusionState *state, const uint8_t bytes[4])
{
  uint8_t command = bytes[2];
  state->display = command;
  state->hex_display = 1;
  if (state->learning) {
    if (state->learn_index != 0 && state->address != bytes[0]) return 0;
    for (unsigned i = 0; i < state->learn_index; ++i)
      if (state->keys[i] == command) return 0;
    state->address = bytes[0];
    state->keys[state->learn_index++] = command;
    if (state->learn_index == 5) {
      state->learning = 0; state->ready = 1;
      return 2;
    }
    return 1;
  }
  if (!state->ready || state->address != bytes[0]) return 0;
  if (command == state->keys[0]) state->led4 ^= 1U;
  for (unsigned i = 1; i < 5; ++i)
    if (command == state->keys[i]) {
      state->mode = (uint8_t)i;
      state->step = 0;
      break;
    }
  return 0;
}

void Fusion_ButtonIncrement(FusionState *state)
{
  state->display = (uint8_t)((state->display + 1U) % 100U);
  state->hex_display = 0;
}

uint8_t Fusion_NextLED(FusionState *state)
{
  static const uint8_t patterns[4][10] = {
    {0,128,64,32,16,8,4,2,1,0}, {0,1,2,4,8,16,32,64,128,0},
    {0,255,0}, {0,129,66,36,24,24,36,66,129,0}};
  static const uint8_t lengths[4] = {10,10,3,10};
  uint8_t index = (uint8_t)(state->mode - 1U);
  if (index >= 4U) return 0;
  uint8_t value = patterns[index][state->step];
  if (++state->step >= lengths[index]) state->step = 0;
  return value;
}

uint8_t Fusion_Digit(const FusionState *state, uint8_t units)
{
  if (state->hex_display)
    return units ? (state->display & 15U) : (state->display >> 4);
  return units ? (state->display % 10U) : ((state->display / 10U) % 10U);
}
