# Lab1_Fusion_Bai4

Project STM32CubeIDE mới kết hợp hai project gốc để làm **mục 4 của Thực hành 1**.
Hai thư mục `Lab1_20242` và `IrRemoteControl/IrRemoteControl` không bị chỉnh sửa.
Các file main/driver ban đầu được lưu thêm tại `docs/original_sources` để đối chiếu.

## Hoạt động

- Ban đầu LED 7 thanh hiển thị số thập phân **25**, dãy LED đơn chạy hiệu ứng 1.
- Mỗi khung NEC hợp lệ hiển thị **byte lệnh thứ ba dưới dạng HEX 00–FF**. Ví dụ byte 0xAD hiển thị `Ad`; không cắt giá trị 173 thành 73.
- Sau khi học mã: nút OK đảo LED4 (PG14); nút 1, 2, 3, 4 chọn hiệu ứng tương ứng trên PD8–PD15. Hiệu ứng đã chọn lặp liên tục đến khi chọn nút khác.
- Khác phần demo 3.1 tự chuyển sau 2 chu kỳ: bài 4 giữ hiệu ứng do remote chọn.
- Giữ nút remote không lặp hành động OK: khung NEC repeat được bỏ qua.
- B1 bấm ngắn: tăng số thập phân, 99 về 00. Giữ B1 2 giây: học lại mã remote.
- Quét LED 7 thanh mỗi 2 ms, hiệu ứng LED mỗi 150 ms; UART không dùng chờ blocking.

## Học mã remote

PDF và project gốc không có bảng mã OK/1/2/3/4, nên không gán mã phỏng đoán.

1. Nạp firmware; giữ B1 khoảng 2 giây rồi nhả. LED3 nhấp nháy.
2. Bấm và nhả lần lượt **OK → 1 → 2 → 3 → 4**, mỗi nút một lần.
3. LED3 sáng liên tục khi học xong; Hercules báo `READY` và bảng mã.
4. Thử OK và 1–4. Các nút không được ánh xạ vẫn hiển thị byte lệnh nhưng không đổi LED4/hiệu ứng.

**Mã học lưu trong RAM:** sau reset hoặc mất điện phải học lại. Có thể giữ B1 2 giây để bắt đầu lại nếu bấm nhầm.
Trong lúc học, mã trùng và địa chỉ remote khác bị từ chối.

## Nối dây

| Chức năng | Chân STM32 |
|---|---|
| 8 LED đơn, qua điện trở theo PDF | PD8–PD15 |
| LED 7 thanh A, B, C, D, E, F, G, DP | PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15 |
| Transistor chọn hàng chục / hàng đơn vị | PG2 / PG3 |
| HS0038 S / − / + | PG5 / GND / 3V |
| B1 có sẵn trên kit | PA0 |
| LED3 / LED4 có sẵn trên kit | PG13 / PG14 |
| USART1 TX / RX | PA9 / PA10 |

Hai LED 7 thanh là common-cathode, dùng transistor NPN theo Hình 2 của PDF.
Khi nhận lệnh HEX, các ký tự A, b, C, d, E, F sử dụng đúng các thanh tương ứng.

Để xem Hercules: dùng kết nối UART TTL 3.3V, PA9 → RX adapter, PA10 → TX adapter nếu cần, GND chung.
Chọn đúng COM, **115200 baud, 8 data bits, no parity, 1 stop bit, no flow control**.
Cổng USB ST-LINK dùng nạp/debug; việc có cổng COM không đồng nghĩa PA9/PA10 đã được nối tới cổng đó.

## Import và nạp bằng CubeIDE

1. File → Import → General → Existing Projects into Workspace.
2. Chọn thư mục `C:\Users\yoga\Downloads\Lab1_Fusion_Bai4`.
3. Chọn project `Lab1_Fusion_Bai4`; không import file `.eww`.
4. Build Project. Cấu hình Debug tạo `Debug/Lab1_Fusion_Bai4.elf`.
5. Chọn project → Debug As → STM32 C/C++ Application, dùng ST-LINK/SWD.
6. Nếu dừng ở main, nhấn Resume để chương trình chạy, sau đó học mã remote.

Có sẵn ELF/HEX/BIN trong `Debug`. Có thể nạp HEX bằng STM32CubeProgrammer;
nếu nạp BIN, địa chỉ flash là **0x08000000**.

## Cấu trúc

- `Core/Src/main.c`: một entry point duy nhất, clock/GPIO/TIM6/USART1 và vòng lặp chung.
- `fusion_app.c`: quét LED 7 thanh, B1, LED đơn, hàng đợi UART và xử lý lệnh.
- `fusion_logic.c`: giải mã NEC LSB-first, kiểm tra hai byte đảo, học mã và ánh xạ hành động.
- `irda.c`: nhận cạnh PG5 bằng EXTI, chuyển khung hợp lệ từ ngắt sang main bằng hàng đợi.
- `stm32f4xx_it.c`: ngắt TIM6 100 us và EXTI9_5.

HSE 8 MHz, CPU 180 MHz, APB1 timer 90 MHz, TIM6 PSC=89/ARR=99 → 10 kHz.
Chỉ hỗ trợ NEC chuẩn có byte địa chỉ đảo và byte lệnh đảo; NEC mở rộng không được chấp nhận.

## Kiểm tra

`tests/run_tests.py` biên dịch logic C cho Cortex-M4 rồi chạy bằng Unicorn ARM.
Các test bao gồm khung hợp lệ/sai, thứ tự bit/byte, jitter, khung repeat, phục hồi sau nhiễu,
học mã/loại mã trùng, OK/1–4, toàn bộ byte 00–FF và bốn chuỗi hiệu ứng.
Máy chạy test cần bộ ARM GCC và `python -m pip install unicorn`.

Build tự động CubeIDE xác nhận khả năng biên dịch/link; chưa chứng minh dây nối hoặc hoạt động trên kit.
Sau nạp cần kiểm tra 25 → B1 tăng số → học remote → mã HEX → OK đảo LED4 → 1–4 đổi hiệu ứng.
