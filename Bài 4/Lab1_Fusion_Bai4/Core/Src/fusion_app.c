#include "main.h"
#include "fusion_app.h"
#include "fusion_logic.h"
#include "irda.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
static FusionState app;
static uint32_t scan_tick, led_tick, status_tick;
static uint8_t scan_units;
static GPIO_PinState button_raw, button_stable;
static uint32_t button_changed, button_pressed;
static uint8_t long_press_fired;
static const char *const learning_names[5] = {"OK", "1", "2", "3", "4"};
static const uint8_t segments[16] = {
  0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
  0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71};

/* Foreground-only ring: no UART waiting and no DMA/interrupt dependency.
   USART1 is owned exclusively by this transmitter. */
static char uart_queue[512];
static unsigned uart_head, uart_tail;
static void UART_Queue(const char *text)
{
  unsigned length = (unsigned)strlen(text);
  unsigned free_space = (uart_tail + sizeof(uart_queue) - uart_head - 1U)
                         % sizeof(uart_queue);
  if (length > free_space) return;
  while (*text) {
    uart_queue[uart_head] = *text++;
    uart_head = (uart_head + 1U) % sizeof(uart_queue);
  }
}
static void UART_Run(void)
{
  if (uart_tail != uart_head && __HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE)) {
    huart1.Instance->DR = (uint8_t)uart_queue[uart_tail];
    uart_tail = (uart_tail + 1U) % sizeof(uart_queue);
  }
}
static void BeginLearning(void)
{
  Fusion_BeginLearning(&app);
  UART_Queue("LEARN: press OK, then 1, 2, 3, 4 (one press each).\r\n");
}
static void Button_Run(uint32_t now)
{
  GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  if (raw != button_raw) { button_raw = raw; button_changed = now; }
  if ((uint32_t)(now - button_changed) >= 30U && button_stable != raw) {
    button_stable = raw;
    if (raw == GPIO_PIN_SET) {
      button_pressed = now;
      long_press_fired = 0;
    } else if (!long_press_fired && !app.learning) {
      Fusion_ButtonIncrement(&app);
    }
  }
  if (button_stable == GPIO_PIN_SET && !long_press_fired &&
      (uint32_t)(now - button_pressed) >= 2000U) {
    long_press_fired = 1;
    BeginLearning();
  }
}
static void Display_Run(uint32_t now)
{
  if ((uint32_t)(now - scan_tick) < 2U) return;
  scan_tick = now;
  /* Blank both modules before changing segment data to avoid ghosting. */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
  uint8_t digit = Fusion_Digit(&app, scan_units);
  GPIOE->BSRR = ((uint32_t)0xFF00U << 16) | ((uint32_t)segments[digit] << 8);
  HAL_GPIO_WritePin(GPIOG, scan_units ? GPIO_PIN_3 : GPIO_PIN_2, GPIO_PIN_SET);
  scan_units ^= 1U;
}
static void Commands_Run(void)
{
  uint8_t bytes[4];
  if (!IrTakeCommand(bytes)) return;
  uint8_t old_index = app.learn_index;
  int result = Fusion_Command(&app, bytes);
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, app.led4 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  char message[160];
  if (result == 2) {
    snprintf(message, sizeof(message),
      "%02X%02X%02X%02X CMD=%02X; learned %s. READY\r\n"
      "KEYS OK=%02X 1=%02X 2=%02X 3=%02X 4=%02X (RAM only)\r\n",
      bytes[0],bytes[1],bytes[2],bytes[3],bytes[2],learning_names[old_index],
      app.keys[0],app.keys[1],app.keys[2],app.keys[3],app.keys[4]);
  } else if (result == 1) {
    snprintf(message, sizeof(message),
      "%02X%02X%02X%02X CMD=%02X; learned %s; press %s next.\r\n",
      bytes[0],bytes[1],bytes[2],bytes[3],bytes[2],
      learning_names[old_index],learning_names[app.learn_index]);
  } else {
    snprintf(message, sizeof(message), "%02X%02X%02X%02X CMD=%02X%s\r\n",
      bytes[0],bytes[1],bytes[2],bytes[3],bytes[2],
      app.learning ? "; duplicate/wrong remote, try requested key again" : "");
  }
  UART_Queue(message);
}
void FusionApp_Init(void)
{
  Fusion_Init(&app);
  IrInit();
  scan_tick = led_tick = status_tick = HAL_GetTick();
  button_raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  button_stable = GPIO_PIN_RESET;
  button_changed = HAL_GetTick();
  UART_Queue("Bai 4 fusion: USART1 115200 8N1. Display starts at decimal 25.\r\n"
             "IR CMD displays HEX 00-FF. Hold B1 2s to learn OK,1,2,3,4.\r\n"
             "Mapping is RAM only: learn again after reset.\r\n");
}
void FusionApp_Run(void)
{
  uint32_t now = HAL_GetTick();
  Button_Run(now);
  Commands_Run();
  Display_Run(now);
  if ((uint32_t)(now - led_tick) >= 150U) {
    led_tick = now;
    uint8_t value = Fusion_NextLED(&app);
    GPIOD->BSRR = ((uint32_t)0xFF00U << 16) | ((uint32_t)value << 8);
  }
  if (app.learning) {
    if ((uint32_t)(now - status_tick) >= 250U) {
      status_tick = now;
      HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_13);
    }
  } else {
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, app.ready ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
  UART_Run();
}
