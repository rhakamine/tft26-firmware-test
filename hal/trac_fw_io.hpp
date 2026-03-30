#pragma once
// ═════════════════════════════════════════════════════════════════════════════
//  trac_fw_io.hpp  —  Tractian Firmware I/O
// ─────────────────────────────────────────────────────────────────────────────
//  Instantiate exactly one trac_fw_io_t in main() and keep it alive for the
//  entire duration of the firmware execution.
//
//  ┌─────────────────────────────────────────────────────────────────────────┐
//  │                         REGISTER MAP                                    │
//  ├──────────────┬──────────────────────────────────────────────────────────┤
//  │  OUT[0]      │  Digital outputs  — bit N = port N   (1 = HIGH, 0 = LOW) │
//  │  OUT[1]      │  PWM channel 0    — duty cycle 0 … 1000                  │
//  │  OUT[2]      │  PWM channel 1    — duty cycle 0 … 1000                  │
//  │  OUT[3..7]   │  reserved                                                │
//  ├──────────────┼──────────────────────────────────────────────────────────┤
//  │  IN[0]       │  Digital inputs   — bit N = switch / button N            │
//  │  IN[1]       │  ADC channel 0    — 0 … 4095  (12-bit)                   │
//  │  IN[2]       │  ADC channel 1    — 0 … 4095  (12-bit)                   │
//  │  IN[3..7]    │  reserved                                                │
//  └──────────────┴──────────────────────────────────────────────────────────┘
// ═════════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <functional>

#define TMP64_ADDRESS 0x48
#define REG_WHO_AM_I 0x0F
#define TMP_MSB 0x00
#define TMP_B2  0x01
#define TMP_B1  0x02
#define TMP_LSB 0x03

#define HMD_MSB 0x00
#define HMD_B2  0x01
#define HMD_B1  0x02
#define HMD_LSB 0x03


#define SCL 8
#define SDA 9
#define DELAY 1

// Trigger modes for attach_interrupt().
enum class InterruptMode { RISING, FALLING, CHANGE };

class trac_fw_io_t {
public:
    // ── object lifecycle ────────────────────────────────────────────────────
    trac_fw_io_t();
    ~trac_fw_io_t();

    trac_fw_io_t(const trac_fw_io_t&)            = delete;
    trac_fw_io_t& operator=(const trac_fw_io_t&) = delete;

    // ── digital I/O  ────────────────────────────────────────────────────────
    //  port : 0 – 31
    void digital_write(uint8_t port, bool level);
    bool digital_read (uint8_t port) const;

    // ── GPIO pull-up  ────────────────────────────────────────────────────────
    //  Enables or disables the internal pull-up resistor on the given port.
    //  port : 0 – 31
    void set_pullup(uint8_t port, bool enable);

    // ── PWM output  ─────────────────────────────────────────────────────────
    //  channel : 0 or 1   |   duty : 0 (off) … 1000 (full on)
    void pwm_write(uint8_t channel, uint16_t duty);

    // ── ADC input  ──────────────────────────────────────────────────────────
    //  channel : 0 or 1   |   returns : 0 – 4095
    uint16_t analog_read(uint8_t channel) const;

    // ── timing  ─────────────────────────────────────────────────────────────
    uint32_t millis() const;
    void     delay(uint32_t ms) const;

    // ── raw register access (advanced)  ─────────────────────────────────────
    uint32_t read_reg (uint8_t idx) const;
    void     write_reg(uint8_t idx, uint32_t value);

    // ── interrupt I/O  ───────────────────────────────────────────────────────
    //  Registers a callback fired on the specified digital port edge.
    //  The callback runs in the HAL receive thread (ISR context) — any state
    //  shared with main() must be protected with std::atomic<> or a mutex.
    //
    //  port : 0 – 31   mode : RISING | FALLING | CHANGE
    void attach_interrupt(uint8_t port,
                          std::function<void()> callback,
                          InterruptMode mode);
    void detach_interrupt(uint8_t port);

    void i2c_start()
    {
        this->digital_write(SCL, 1);
        this->digital_write(SDA, 1);
        this->delay(DELAY);
        this->digital_write(SDA, 0);
        this->delay(DELAY);
        this->digital_write(SCL, 0);
        this->delay(DELAY);
    }

    void i2c_stop()
    {
        this->digital_write(SDA, 0);
        this->delay(DELAY);
        this->digital_write(SCL, 1);
        this->delay(DELAY);
        this->digital_write(SDA, 1);
        this->delay(DELAY);
    }

    uint8_t i2c_write_byte (uint8_t data)
    {
        uint8_t bit;
        uint8_t ack;
        for (bit = 0; bit < 8; bit++)
        {
            if (data & 0x80)
            {
                this->digital_write(SDA, 1);
            }
            else
            {
                this->digital_write(SDA, 0);
            }
            data <<= 1;

            this->delay(DELAY);
            this->digital_write(SCL, 1);
            this->delay(DELAY);
            this->digital_write(SCL, 0);
        }
        this->digital_write(SDA, 1);
        this->delay(DELAY);
        this->digital_write(SCL, 1);
        this->delay(DELAY);

        ack = this->digital_read(SDA);

        this->digital_write(SCL, 0);
        this->delay(DELAY);
        return ack;
    }

    uint8_t i2c_read_byte(uint8_t ack)
    {
        uint8_t data = 0;
        uint8_t bit;

        for (bit = 0; bit < 8; bit++)
        {
            data <<= 1;
            this->digital_write(SCL, 1);
            this->delay(DELAY);

            data |= this->digital_read(SDA);

            this->digital_write(SCL, 0);
            this->delay(DELAY);
        }

        if (ack)
        {
            this->digital_write(SDA, 0);
        }
        else
        {
            this->digital_write(SDA, 1);
        }

        this->delay(DELAY);
        this->digital_write(SCL, 1);
        this->delay(DELAY);
        this->digital_write(SCL, 0);
        this->delay(DELAY);

        this->digital_write(SDA, 1);
        this->delay(DELAY);
        return data;
    }

private:
    struct _hw_t;   // opaque — implementation detail
    _hw_t* _hw;

};
