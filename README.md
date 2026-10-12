# Thực hành 1 — STM32F429-DISC

Repo gồm hai project STM32CubeIDE:

- **Bài 3.1 và 3.2/**: ghép nối LED đơn, bốn hiệu ứng, LED 7 thanh và tăng số bằng B1; project `Lab1_20242`.
- **Bài 4/**: giải mã NEC, hiển thị byte lệnh trên LED 7 thanh, nút OK điều khiển LD4/PG14 và nút 1–4 chọn hiệu ứng; project `Lab1_Fusion_Bai4`.

## Mở project

Trong STM32CubeIDE: File → Import → General → Existing Projects into Workspace, chọn folder bài tương ứng.
Tên folder trên GitHub được đặt theo bài; tên project bên trong IDE giữ nguyên để giữ cấu hình hiện có.

Toàn bộ file từ hai thư mục project trên máy được giữ lại, gồm source, Drivers, cấu hình CubeIDE/CubeMX, Debug/Release và firmware đã có.
Không thay đổi các project gốc khi chuẩn bị repo.

## Bài 4

Xem `Bài 4/README.md` để nối dây, UART và học mã remote.
Bản hiện tại có chế độ học mã trong RAM: giữ B1 2 giây, nhả rồi bấm OK → 1 → 2 → 3 → 4.
LED 7 thanh hiển thị byte lệnh ở dạng hex 00–FF.
Build đã được kiểm tra; hoạt động thực tế của remote/LD4 cần tiếp tục kiểm tra trên kit.
