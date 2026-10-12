/*
 * irda.c
 *
 *  Created on: Nov 1, 2024
 *      Author: trung
 */
#include "irda.h"
#include "main.h"
#include "fusion_logic.h"

volatile uint32_t tim6_count;
static NEC_Decoder decoder;
static uint32_t last_edge;
static uint8_t first_edge;
static volatile uint8_t frames[4][4];
static volatile uint8_t head, tail;

void IrInit(void)
{
  decoder.frame = 0; decoder.bits = 0; decoder.active = 0;
  tim6_count = 0; last_edge = 0; first_edge = 1; head = tail = 0;
}
void IrGetBitTime(void)
{
  uint32_t now = tim6_count;
  uint32_t interval = now - last_edge;
  last_edge = now;
  if (first_edge) { first_edge = 0; return; }
  uint8_t bytes[4];
  if (NEC_FeedInterval(&decoder, interval, bytes)) {
    uint8_t next = (uint8_t)((head + 1U) % 4U);
    if (next != tail) {
      for (unsigned i = 0; i < 4; ++i) frames[head][i] = bytes[i];
      __DMB();
      head = next;
    }
  }
}
int IrTakeCommand(uint8_t bytes[4])
{
  uint32_t saved = __get_PRIMASK();
  __disable_irq();
  int ready = (head != tail);
  if (ready) {
    for (unsigned i = 0; i < 4; ++i) bytes[i] = frames[tail][i];
    tail = (uint8_t)((tail + 1U) % 4U);
  }
  __set_PRIMASK(saved);
  return ready;
}
