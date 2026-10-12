# Bai 4 fusion implementation plan
Goal: combine the two projects without changing either source directory.
Architecture: NEC interrupt capture + pure decoder/remote logic + foreground display/UART tasks.
Constraints: STM32F429DISC; HSE 8 MHz; CPU 180 MHz; TIM6 10 kHz; UART1 PA9/PA10 115200 8N1.
1. Create an isolated copy and snapshot/hash original projects.
2. Test the real ARM decoder and application logic with emulated valid/invalid NEC frames, repeat handling, learning, mapping and display/LED patterns.
3. Merge main and GPIO; integrate the IR ISR without blocking display scanning; preserve original main files in docs.
4. Rename IDE metadata, set explicit source exclusions and synchronize IOC.
5. Build Debug/Release firmware, review integration, confirm original hashes and document wiring/import/learning.
