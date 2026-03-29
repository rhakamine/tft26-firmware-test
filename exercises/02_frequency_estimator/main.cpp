// =============================================================================
//  Challenge 02 — Frequency Estimator
// =============================================================================
//
//  Virtual hardware:
//    ADC Ch 0  →  io.analog_read(0)      Process sensor signal (0–4095)
//    OUT reg 3 →  io.write_reg(3, …)     Frequency estimate in centiHz
//                                        e.g. write_reg(3, 4733) = 47.33 Hz
//
//  Goal:
//    Measure the frequency of the signal on ADC channel 0 and publish your
//    estimate continuously via register 3.
//
//  Read README.md before starting.
// =============================================================================

#include <trac_fw_io.hpp>
#include <cstdint>


int main() {
    trac_fw_io_t io;
    const uint32_t period_sampling_ms = 10;

    int n_points = 100;
    int n_points_for_average = 5;
    int n_filtered_points = n_points - n_points_for_average + 1;

    double points[n_points];
    double filtered_points[n_filtered_points];

    int counter = 0;
    double frequency = 0.0;

    uint32_t start_time = io.millis();

    while (true) {
        uint32_t delta = io.millis() - start_time;
        if (delta >= period_sampling_ms)
        {
            points[counter] = io.analog_read(0);
            start_time = io.millis();
        }

        counter++;
        if (counter >= n_points) 
        {
            double acc = 0.0;
            double max_value = points[0];
            double min_value = points[0];
            double middle = 0.0;
            for (int i = 0; i < n_points; i++)
            {
                if (i >= n_points_for_average)
                {
                    acc -= points[i - n_points_for_average];
                    acc += points[i];
                    filtered_points[i - n_points_for_average] = acc / n_points_for_average;
                }
                else
                {
                    acc += points[i];
                }
                
                if (points[i] > max_value) { max_value = points[i]; }
                if (points[i] < min_value) { min_value = points[i]; }
            }
            middle = (max_value + min_value) / 2;

            int zero_crossings = 0;
            for (int i = 1; i < n_filtered_points; i++)
            {
                if (filtered_points[i-1] <= middle && filtered_points[i] > middle)
                {
                    zero_crossings++;
                }
            }

            double duration = (double)n_filtered_points * period_sampling_ms / 1000.0;

            frequency = (double)zero_crossings / duration;
            counter = 0;
        }
        io.write_reg(3, frequency * 100.0);
        io.delay(10);
    }
}
