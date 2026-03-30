// =============================================================================
//  Exercise 03 — I2C Sensors (Bit-bang)
// =============================================================================
//
//  Virtual hardware:
//    P8 (SCL)  →  io.digital_write(8, …) / io.digital_read(8)
//    P9 (SDA)  →  io.digital_write(9, …) / io.digital_read(9)
//
//  PART 1 — TMP64 temperature sensor at I2C address 0x48
//    Register 0x0F  WHO_AM_I   — 1 byte  (expected: 0xA5)
//    Register 0x00  TEMP_RAW   — 4 bytes, big-endian int32_t, milli-Celsius
//
//  PART 2 — Unknown humidity sensor (same register layout, address unknown)
//    Register 0x0F  WHO_AM_I   — 1 byte
//    Register 0x00  HUM_RAW    — 4 bytes, big-endian int32_t, milli-percent
//
//  Goal (Part 1):
//    1. Implement an I2C master via bit-bang on P8/P9.
//    2. Read WHO_AM_I from TMP64 and confirm the sensor is present.
//    3. Read TEMP_RAW in a loop and print the temperature in °C every second.
//    4. Update display registers 6–7 with the formatted temperature string.
//
//  Goal (Part 2):
//    5. Scan the I2C bus (addresses 0x08–0x77) and print every responding address.
//    6. For each unknown device found, read its WHO_AM_I and print it.
//    7. Add the humidity sensor to the 1 Hz loop: read HUM_RAW and print %RH.
//
//  Read README.md before starting.
// =============================================================================

#include <trac_fw_io.hpp>
#include <cstdio>
#include <cstdint>
#include <cstring>


/**
 * For the "merry-go-round" display of the data, the io.millis() was used
 * together with the count of how many humidity sensors were found, displaying
 * each of its value by (1 second / (number of humidity sensors)) second 
 */
int main() {
    trac_fw_io_t io;

    // Initialize I2C
    io.set_pullup(SDA, 1);
    io.set_pullup(SCL, 1);

    // Verify temp sensor ID
    io.i2c_start();
    io.i2c_write_byte((TMP64_ADDRESS << 1) | 0);
    io.i2c_write_byte(REG_WHO_AM_I);

    io.i2c_start();
    io.i2c_write_byte((TMP64_ADDRESS << 1) | 1);
    uint8_t id = io.i2c_read_byte(0);
    std::printf("Temp sensor ID: %X\n", id);

    // BUS Scan for HMD sensors
    uint8_t hmd_addrs[0x77 - 0x08];
    uint8_t count_hmd_sensors = 0;
    for (uint32_t addr = 0x08; addr <= 0x77; addr++)
    {
        uint8_t ack = 0;
        io.i2c_start();
        ack = io.i2c_write_byte((addr << 1) | 0);
        if (ack == 0)
        {
            std::printf("HMD10 %d ADDR: %X\n", count_hmd_sensors + 1 ,addr);
            hmd_addrs[count_hmd_sensors] = addr;
            count_hmd_sensors++;
        }
    }

    // Print found HMD sensors
    for (uint8_t i = 0; i < count_hmd_sensors; i++)
    {
        io.i2c_start();
        io.i2c_write_byte((hmd_addrs[i] << 1) | 0);
        io.i2c_write_byte(REG_WHO_AM_I);

        io.i2c_start();
        io.i2c_write_byte((hmd_addrs[i]<< 1) | 1);
        uint8_t id = io.i2c_read_byte(0);
        std::printf("HMD sensor %d ID: %X\n", i + 1, id);
    }

    char temp_buf[9] = {};
    char hum_buf[9] = {};
    double temp_c = 0.0, hum_pct = 0.0;
    uint8_t data = 0;
    uint32_t temp_c_bytes = 0;
    uint32_t hum_bytes = 0;

    uint32_t r4, r5, r6, r7;
    uint32_t start_time_tmp = io.millis();
    uint32_t start_time_hmd = io.millis();
    uint32_t delta_t_tmp = 0,
             delta_t_hmd = 0;
    uint8_t hmd_sensor_i = 0;
    while (true) {
        // Temp sensor
        delta_t_tmp = io.millis() - start_time_tmp;
        if (delta_t_tmp >= 1000)
        {
            io.i2c_start();
            io.i2c_write_byte((TMP64_ADDRESS << 1) | 0);
            io.i2c_write_byte(TMP_MSB);
            io.i2c_start();
            io.i2c_write_byte((TMP64_ADDRESS << 1) | 1);
            temp_c = 0.0;
            temp_c_bytes = 0;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (i < 3)
                {
                    data = io.i2c_read_byte(1);
                }
                else
                {
                    data = io.i2c_read_byte(0);
                }
                temp_c_bytes |= data << (8 * (3 - i));
            }
            temp_c = (double)temp_c_bytes / 1000.0;
            std::printf("TEMP: %f\n", temp_c);
            std::snprintf(temp_buf, sizeof(temp_buf), "%8.3f", temp_c);
            std::memcpy(&r6, temp_buf + 0, 4);
            std::memcpy(&r7, temp_buf + 4, 4);
            io.write_reg(6, r6);
            io.write_reg(7, r7);
            start_time_tmp = io.millis();
        }

        // Humidity sensors
        delta_t_hmd = io.millis() - start_time_hmd;
        if (delta_t_hmd >= (1000 / count_hmd_sensors))
        {
            io.i2c_start();
            io.i2c_write_byte((hmd_addrs[hmd_sensor_i] << 1) | 0);
            io.i2c_write_byte(HMD_MSB);
            io.i2c_start();
            io.i2c_write_byte((hmd_addrs[hmd_sensor_i] << 1) | 1);
            hum_pct = 0.0;
            hum_bytes = 0;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (i < 3)
                {
                    data = io.i2c_read_byte(1);
                }
                else
                {
                    data = io.i2c_read_byte(0);
                }
                hum_bytes |= data << (8 * (3 - i));
            }
            hum_pct = (double)hum_bytes / 1000.0;
            std::printf("Humidity sensor %X: %7.3f%%\n", hmd_addrs[hmd_sensor_i], hum_pct);

            std::snprintf(hum_buf, sizeof(hum_buf), "%7.3f%%", hum_pct);
            std::memcpy(&r4, hum_buf + 0, 4);
            std::memcpy(&r5, hum_buf + 4, 4);
            io.write_reg(4, r4);
            io.write_reg(5, r5);
            hmd_sensor_i++;
            if (hmd_sensor_i == count_hmd_sensors)
            {
                hmd_sensor_i = 0;
            }
            start_time_hmd = io.millis();
        }

        io.delay(10);
    }
}
