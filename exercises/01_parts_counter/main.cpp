// =============================================================================
//  Exercise 01 — Parts Counter
// =============================================================================
//
//  Virtual hardware:
//    SW 0        →  io.digital_read(0)        Inductive sensor input
//    Display     →  io.write_reg(6, …)        LCD debug (see README for format)
//                   io.write_reg(7, …)
//
//  Goal:
//    Count every part that passes the sensor and show the total on the display.
//
//  Read README.md before starting.
// =============================================================================

#include <trac_fw_io.hpp>
#include <cstdint>

#include <cstring>
#include <cstdio>

#define POSITIVE_N_POINTS 10
#define NEGATIVE_N_POINTS 30

typedef enum state {
    IDLE = 0,
    BEGIN_CHECK,
    HAS_OBJECT,
    LEFT_OBJECT
} state_t;


int main() {
    trac_fw_io_t io;

    uint32_t count = 0;

    char buf[9] = {};
    uint32_t r6, r7;
    bool sensor_read = false;

    uint32_t positive_counter = 0,
             reads_counter = 0;

    state_t state = IDLE;

    while (true) {
        sensor_read = io.digital_read(0);

        switch (state) {
            case (IDLE):
                if (sensor_read) {
                    positive_counter++;
                    reads_counter++;
                    state = BEGIN_CHECK;
                }
                break;

            case (BEGIN_CHECK):
                reads_counter++;
                if (sensor_read) { positive_counter++; }
                else if (reads_counter >= POSITIVE_N_POINTS)
                {
                    if (positive_counter >= (POSITIVE_N_POINTS * 0.7f))
                    {
                        state = HAS_OBJECT;
                    }
                    positive_counter = 0;
                    reads_counter = 0;
                }
                break;

            case (HAS_OBJECT):
                reads_counter++;
                if (sensor_read) { positive_counter++; }
                if (positive_counter >= (NEGATIVE_N_POINTS * 0.25f))
                {
                    positive_counter = 0;
                    reads_counter = 0;
                }
                else if (reads_counter >= NEGATIVE_N_POINTS)
                {
                    state = LEFT_OBJECT;
                    positive_counter = 0;
                    reads_counter = 0;
                }
                break;

            case (LEFT_OBJECT):
                count++;
                state = IDLE;
                break;

            default:
                state = IDLE;
                break;
        }

        std::snprintf(buf, sizeof(buf), "%8u", count); // right-aligned, 8 chars
        std::memcpy(&r6, buf + 0, 4);
        std::memcpy(&r7, buf + 4, 4);
        io.write_reg(6, r6);
        io.write_reg(7, r7);

        io.delay(1);
    }
}
