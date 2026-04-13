# Startup stability rewrite

## Mục tiêu đạt được
- Dồn toàn bộ giai đoạn ổn định cảm biến vào màn hình loading.
- Không cho người dùng nhìn thấy dữ liệu Euler trong lúc gyro bias và quaternion còn đang hội tụ.
- Khi vào `Luyện tập cá nhân` hoặc `Luyện tập đội hình`, góc Pitch / Roll / Yaw đã ở trạng thái sẵn sàng sử dụng.

## Thay đổi chính
1. **Màn loading không còn là timer giả**
   - `screen_loading.c/.h` được đổi sang chế độ nhận progress thật từ backend.
   - Chỉ rời loading khi IMU đã hiệu chuẩn và warm-up xong.

2. **Viết lại startup IMU**
   - `IMUWrapper` giờ có bước:
     - flush các mẫu đầu sau khi bật sensor,
     - chỉ nhận các mẫu **đứng yên liên tiếp**,
     - tính lại `gyroBias` từ cửa sổ ổn định,
     - seed trước bộ lọc accel để tránh nhảy từ 0 lên giá trị thật.
   - Có thêm cập nhật bias gyro chậm khi thiết bị đứng yên để giảm drift đầu phiên.

3. **Khởi tạo quaternion từ vector gravity**
   - `MadgwickFilter` có `initializeFromAccel(...)` để roll/pitch đúng ngay từ đầu,
     thay vì luôn bắt đầu từ quaternion identity.
   - Beta của Madgwick được làm mượt theo trạng thái tĩnh/chuyển động.

4. **Warm-up filter trước khi cho UI dùng dữ liệu**
   - `AppTracking::prepareForUse()` chạy warm-up nội bộ, theo dõi độ biến thiên Euler.
   - Chỉ khi chuỗi mẫu đạt tiêu chí ổn định mới đánh dấu `startupReady = true`.

5. **Reset session khi mở bài tập**
   - Thêm `application_prepare_session()` và gọi ở `screen_main.c`.
   - Khi người dùng bấm vào chế độ luyện tập, phần position/motion được reset lại,
     nhưng orientation vẫn giữ trạng thái đã ổn định sẵn.

## File đã sửa
- `main/application.cpp`
- `main/application.h`
- `main/imu_process/AppTracking.cpp`
- `main/imu_process/AppTracking.h`
- `main/imu_process/IMUWrapper.cpp`
- `main/imu_process/IMUWrapper.h`
- `main/imu_process/MadgwickFilter.cpp`
- `main/imu_process/MadgwickFilter.h`
- `main/imu_process/TrackingConfig.h`
- `main/ui/screens/screen_loading.c`
- `main/ui/screens/screen_loading.h`
- `main/ui/screens/screen_main.c`
- `main/ui/components/ui_header.h`

## Ghi chú kỹ thuật
- Với IMU 6 trục không có magnetometer, **yaw tuyệt đối** vẫn không thể khóa tuyệt đối lâu dài như hệ có từ kế.
- Bản sửa này xử lý mạnh phần **drift / jump ở đầu phiên**, đồng thời giảm drift yaw nhờ bias gyro tốt hơn và zero yaw sau warm-up.
