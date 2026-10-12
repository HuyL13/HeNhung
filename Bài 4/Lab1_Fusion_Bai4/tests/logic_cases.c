#include "fusion_logic.h"
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

static int feed_frame(NEC_Decoder *d, const uint8_t bytes[4], uint8_t out[4])
{
  int result = NEC_FeedInterval(d, 135, out);
  for (unsigned i = 0; i < 32; ++i)
    result = NEC_FeedInterval(d, (bytes[i / 8] & (1U << (i % 8))) ? 23 : 11, out);
  return result;
}

int test_nec_valid(void)
{
  NEC_Decoder d = {0}; uint8_t out[4] = {0};
  const uint8_t bytes[4] = {0, 255, 0xAD, 0x52};
  CHECK(feed_frame(&d, bytes, out) == 1);
  for (unsigned i = 0; i < 4; ++i) CHECK(out[i] == bytes[i]);
  return 0;
}
#ifdef FULL_TESTS
int test_nec_invalid_complement(void)
{
  NEC_Decoder d = {0}; uint8_t out[4];
  const uint8_t bytes[4] = {0, 255, 0xAD, 0x53};
  CHECK(feed_frame(&d, bytes, out) == 0);
  return 0;
}
int test_nec_invalid_address(void)
{
  NEC_Decoder d = {0}; uint8_t out[4];
  const uint8_t bytes[4] = {0, 254, 0xAD, 0x52};
  CHECK(feed_frame(&d, bytes, out) == 0);
  return 0;
}
int test_nec_noise_repeat_recovery(void)
{
  NEC_Decoder d = {0}; uint8_t out[4];
  const uint8_t bytes[4] = {0, 255, 0x18, 0xE7};
  CHECK(NEC_FeedInterval(&d, 135, out) == 0);
  CHECK(NEC_FeedInterval(&d, 60, out) == 0);
  CHECK(d.active == 0);
  CHECK(NEC_FeedInterval(&d, 113, out) == 0);
  CHECK(feed_frame(&d, bytes, out) == 1);
  CHECK(NEC_FeedInterval(&d, 113, out) == 0);
  return 0;
}
int test_nec_jitter_and_resync(void)
{
  NEC_Decoder d = {0}; uint8_t out[4]; int got;
  NEC_FeedInterval(&d, 125, out);
  for (unsigned i = 0; i < 32; ++i)
    got = NEC_FeedInterval(&d, i >= 8 && i < 16 ? 26 : (i >= 24 ? 20 : 9), out);
  CHECK(got == 1); CHECK(out[0] == 0 && out[1] == 255);
  CHECK(out[2] == 0 && out[3] == 255);
  return 0;
}
static void command(FusionState *s, uint8_t key)
{
  uint8_t bytes[4] = {0, 255, key, (uint8_t)~key}; Fusion_Command(s, bytes);
}
static void learn(FusionState *s)
{
  Fusion_BeginLearning(s);
  for (uint8_t i = 0; i < 5; ++i) command(s, (uint8_t)(0x10 + i));
}
int test_initial_and_decimal_button(void)
{
  FusionState s; Fusion_Init(&s);
  CHECK(s.display == 25 && !s.hex_display);
  CHECK(Fusion_Digit(&s, 0) == 2 && Fusion_Digit(&s, 1) == 5);
  for (unsigned i = 0; i < 75; ++i) Fusion_ButtonIncrement(&s);
  CHECK(s.display == 0); return 0;
}
int test_learning_and_keys(void)
{
  FusionState s; Fusion_Init(&s); learn(&s);
  CHECK(s.ready && !s.learning);
  command(&s, 0x10); CHECK(s.led4 == 1);
  command(&s, 0x10); CHECK(s.led4 == 0);
  for (uint8_t i = 1; i < 5; ++i) {
    command(&s, (uint8_t)(0x10 + i)); CHECK(s.mode == i);
    CHECK(s.step == 0);
  }
  return 0;
}
int test_learning_duplicate_address_and_unmapped(void)
{
  FusionState s; Fusion_Init(&s); Fusion_BeginLearning(&s);
  command(&s, 0x10); command(&s, 0x10); CHECK(s.learn_index == 1);
  uint8_t other[4] = {1, 254, 0x11, 0xEE};
  Fusion_Command(&s, other); CHECK(s.learn_index == 1);
  for (uint8_t i = 1; i < 5; ++i) command(&s, (uint8_t)(0x10 + i));
  CHECK(s.ready);
  command(&s, 0x99); CHECK(s.mode == 1 && !s.led4);
  other[2] = 0x10; other[3] = 0xEF;
  Fusion_Command(&s, other); CHECK(!s.led4);
  return 0;
}
int test_hex_full_byte(void)
{
  FusionState s; Fusion_Init(&s);
  for (unsigned i = 0; i < 256; ++i) {
    command(&s, (uint8_t)i); CHECK(s.display == i && s.hex_display);
    CHECK(Fusion_Digit(&s, 0) == i / 16);
    CHECK(Fusion_Digit(&s, 1) == i % 16);
  }
  return 0;
}
int test_four_effects_hold_selected_mode(void)
{
  const uint8_t expected[4][10] = {
    {0,128,64,32,16,8,4,2,1,0}, {0,1,2,4,8,16,32,64,128,0},
    {0,255,0}, {0,129,66,36,24,24,36,66,129,0}};
  const uint8_t lengths[4] = {10,10,3,10};
  FusionState s; Fusion_Init(&s); learn(&s);
  for (uint8_t mode = 1; mode <= 4; ++mode) {
    command(&s, (uint8_t)(0x10 + mode));
    for (unsigned cycle = 0; cycle < 3; ++cycle)
      for (uint8_t step = 0; step < lengths[mode-1]; ++step)
        CHECK(Fusion_NextLED(&s) == expected[mode-1][step]);
    CHECK(s.mode == mode);
  }
  return 0;
}
#endif
