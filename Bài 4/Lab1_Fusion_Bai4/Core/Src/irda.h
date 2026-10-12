/* NEC receiver for the combined Bai 4 application. */
#ifndef SRC_IRDA_H_
#define SRC_IRDA_H_
#include <stdint.h>
extern volatile uint32_t tim6_count;
void IrInit(void);
void IrGetBitTime(void);
int IrTakeCommand(uint8_t bytes[4]);
#endif
