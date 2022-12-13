// Gnu Radio includes
#include <gnuradio/analog/sig_source.h>
#include <gnuradio/blocks/complex_to_mag_squared.h>
#include <gnuradio/blocks/divide.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/multiply.h>
#include <gnuradio/blocks/multiply_const.h>
#include <gnuradio/blocks/nlog10_ff.h>
#include <gnuradio/blocks/stream_to_vector.h>
#include <gnuradio/blocks/sub.h>
#include <gnuradio/blocks/throttle.h>
#include <gnuradio/blocks/transcendental.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_v.h>
#include <gnuradio/fft/window.h>
#include <gnuradio/filter/fft_filter_fff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/top_block.h>

#include <gnuradio/pulsed_power/opencmw_freq_sink.h>
#include <gnuradio/pulsed_power/opencmw_time_sink.h>
#include <gnuradio/pulsed_power/picoscope_4000a_source.h>
#include <gnuradio/pulsed_power/power_calc_ff.h>
#include <gnuradio/pulsed_power/power_calc_mul_ph_ff.h>

class GRFlowGraphThreePhaseSimulated {
private:
    gr::top_block_sptr top;

public:
    GRFlowGraphThreePhaseSimulated(int noutput_items)
        : top(gr::make_top_block("GNURadio")) {
        // parameters
        float  in_samp_rate   = 4'000.0f;
        float  in_samp_rate_2 = 32'000.0f;
        float  out_samp_rate  = 4'000.0f;
        float  bandwidth      = in_samp_rate_2;
        int    bp_decimation  = 20;
        double bp_high_cut    = 80;
        double bp_low_cut     = 20;
        double bp_trans       = 10;
        // double current_correction_factor = 2.5;
        // double voltage_correction_factor = 100;
        int lp_decimation = 1;

        // blocks
        auto analog_sig_source_current0 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 2, 0, 0.2f);
        auto analog_sig_source_current1 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 2, 0, 0.2f); // 0.2 + pi / 3
        auto analog_sig_source_current2 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 2, 0, 0.2f); // 0.2 + 2*pi / 3

        auto analog_sig_source_voltage0 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 325, 0, 0.0f);
        auto analog_sig_source_voltage1 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 325, 0, 0.0f); // pi/3
        auto analog_sig_source_voltage2 = gr::analog::sig_source_f::make(in_samp_rate, gr::analog::GR_SIN_WAVE, 50, 325, 0, 0.0f); // 2pi/3

        auto band_pass_filter_current0  = gr::filter::fft_filter_fff::make(
                 bp_decimation,
                 gr::filter::firdes::band_pass(
                         1,
                         in_samp_rate,
                         bp_low_cut,
                         bp_high_cut,
                         bp_trans,
                         gr::fft::window::win_type::WIN_HAMMING,
                         6.76));
        auto band_pass_filter_current1 = gr::filter::fft_filter_fff::make(
                bp_decimation,
                gr::filter::firdes::band_pass(
                        1,
                        in_samp_rate,
                        bp_low_cut,
                        bp_high_cut,
                        bp_trans,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto band_pass_filter_current2 = gr::filter::fft_filter_fff::make(
                bp_decimation,
                gr::filter::firdes::band_pass(
                        1,
                        in_samp_rate,
                        bp_low_cut,
                        bp_high_cut,
                        bp_trans,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto band_pass_filter_voltage0 = gr::filter::fft_filter_fff::make(
                bp_decimation,
                gr::filter::firdes::band_pass(
                        1,
                        in_samp_rate,
                        bp_low_cut,
                        bp_high_cut,
                        bp_trans,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto band_pass_filter_voltage1 = gr::filter::fft_filter_fff::make(
                bp_decimation,
                gr::filter::firdes::band_pass(
                        1,
                        in_samp_rate,
                        bp_low_cut,
                        bp_high_cut,
                        bp_trans,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto band_pass_filter_voltage2 = gr::filter::fft_filter_fff::make(
                bp_decimation,
                gr::filter::firdes::band_pass(
                        1,
                        in_samp_rate,
                        bp_low_cut,
                        bp_high_cut,
                        bp_trans,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));

        auto analog_sig_source_phase0_sin = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_SIN_WAVE, 55, 1, 0, 0.0f);
        auto analog_sig_source_phase1_sin = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_SIN_WAVE, 55, 1, 0, 0.0f);
        auto analog_sig_source_phase2_sin = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_SIN_WAVE, 55, 1, 0, 0.0f);
        auto analog_sig_source_phase0_cos = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_COS_WAVE, 55, 1, 0, 0.0f);
        auto analog_sig_source_phase1_cos = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_COS_WAVE, 55, 1, 0, 0.0f);
        auto analog_sig_source_phase2_cos = gr::analog::sig_source_f::make(out_samp_rate, gr::analog::GR_COS_WAVE, 55, 1, 0, 0.0f);

        auto blocks_multiply_phase0_0     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase0_1     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase0_2     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase0_3     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase1_0     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase1_1     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase1_2     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase1_3     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase2_0     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase2_1     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase2_2     = gr::blocks::multiply_ff::make(1);
        auto blocks_multiply_phase2_3     = gr::blocks::multiply_ff::make(1);

        auto low_pass_filter_current0_0   = gr::filter::fft_filter_fff::make(
                  lp_decimation,
                  gr::filter::firdes::low_pass(
                          1,
                          out_samp_rate,
                          60,
                          10,
                          gr::fft::window::win_type::WIN_HAMMING,
                          6.76));
        auto low_pass_filter_current0_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage0_0 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage0_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_current1_0 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_current1_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage1_0 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage1_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_current2_0 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_current2_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage2_0 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));
        auto low_pass_filter_voltage2_1 = gr::filter::fft_filter_fff::make(
                lp_decimation,
                gr::filter::firdes::low_pass(
                        1,
                        out_samp_rate,
                        60,
                        10,
                        gr::fft::window::win_type::WIN_HAMMING,
                        6.76));

        auto blocks_divide_phase0_0                = gr::blocks::divide_ff::make(1);
        auto blocks_divide_phase0_1                = gr::blocks::divide_ff::make(1);
        auto blocks_divide_phase1_0                = gr::blocks::divide_ff::make(1);
        auto blocks_divide_phase1_1                = gr::blocks::divide_ff::make(1);
        auto blocks_divide_phase2_0                = gr::blocks::divide_ff::make(1);
        auto blocks_divide_phase2_1                = gr::blocks::divide_ff::make(1);

        auto blocks_transcendental_phase0_0        = gr::blocks::transcendental::make("atan");
        auto blocks_transcendental_phase0_1        = gr::blocks::transcendental::make("atan");
        auto blocks_transcendental_phase1_0        = gr::blocks::transcendental::make("atan");
        auto blocks_transcendental_phase1_1        = gr::blocks::transcendental::make("atan");
        auto blocks_transcendental_phase2_0        = gr::blocks::transcendental::make("atan");
        auto blocks_transcendental_phase2_1        = gr::blocks::transcendental::make("atan");

        auto blocks_sub_phase0                     = gr::blocks::sub_ff::make(1);
        auto blocks_sub_phase1                     = gr::blocks::sub_ff::make(1);
        auto blocks_sub_phase2                     = gr::blocks::sub_ff::make(1);

        auto pulsed_power_power_calc_mul_ph_ff_0_0 = gr::pulsed_power::power_calc_mul_ph_ff::make(0.0001);

        auto pulsed_power_opencmw_time_sink        = gr::pulsed_power::opencmw_time_sink::make(
                       in_samp_rate,
                       { "P(t)_1", "Q(t)_1", "S(t)_1", "phi(t)_1",
                        "P(t)_2", "Q(t)_2", "S(t)_2", "phi(t)_2",
                        "P(t)_3", "Q(t)_3", "S(t)_3", "phi(t)_3" },
                       {});

        // Connections:
        // Phase 0:
        top->hier_block2::connect(analog_sig_source_current0, 0, band_pass_filter_current0, 0);
        top->hier_block2::connect(analog_sig_source_voltage0, 0, band_pass_filter_voltage0, 0);
        top->hier_block2::connect(band_pass_filter_current0, 0, blocks_multiply_phase0_0, 0);
        top->hier_block2::connect(band_pass_filter_current0, 0, blocks_multiply_phase0_1, 0);
        top->hier_block2::connect(band_pass_filter_voltage0, 0, blocks_multiply_phase0_2, 0);
        top->hier_block2::connect(band_pass_filter_voltage0, 0, blocks_multiply_phase0_3, 0);
        top->hier_block2::connect(band_pass_filter_current0, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 1);
        top->hier_block2::connect(band_pass_filter_voltage0, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 0);
        top->hier_block2::connect(analog_sig_source_phase0_sin, 0, blocks_multiply_phase0_0, 1);
        top->hier_block2::connect(analog_sig_source_phase0_sin, 0, blocks_multiply_phase0_2, 1);
        top->hier_block2::connect(analog_sig_source_phase0_cos, 0, blocks_multiply_phase0_1, 1);
        top->hier_block2::connect(analog_sig_source_phase0_cos, 0, blocks_multiply_phase0_3, 1);
        top->hier_block2::connect(blocks_multiply_phase0_0, 0, low_pass_filter_voltage0_0, 0);
        top->hier_block2::connect(blocks_multiply_phase0_1, 0, low_pass_filter_voltage0_1, 0);
        top->hier_block2::connect(blocks_multiply_phase0_2, 0, low_pass_filter_current0_0, 0);
        top->hier_block2::connect(blocks_multiply_phase0_3, 0, low_pass_filter_current0_1, 0);
        top->hier_block2::connect(low_pass_filter_voltage0_0, 0, blocks_divide_phase0_0, 0);
        top->hier_block2::connect(low_pass_filter_voltage0_1, 0, blocks_divide_phase0_0, 1);
        top->hier_block2::connect(low_pass_filter_current0_0, 0, blocks_divide_phase0_1, 0);
        top->hier_block2::connect(low_pass_filter_current0_1, 0, blocks_divide_phase0_1, 1);
        top->hier_block2::connect(blocks_divide_phase0_0, 0, blocks_transcendental_phase0_0, 0);
        top->hier_block2::connect(blocks_divide_phase0_1, 0, blocks_transcendental_phase0_1, 0);
        top->hier_block2::connect(blocks_transcendental_phase0_0, 0, blocks_sub_phase0, 0);
        top->hier_block2::connect(blocks_transcendental_phase0_1, 0, blocks_sub_phase0, 1);
        top->hier_block2::connect(blocks_sub_phase0, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 2);

        // Phase 1:
        top->hier_block2::connect(analog_sig_source_current1, 0, band_pass_filter_current1, 0);
        top->hier_block2::connect(analog_sig_source_voltage1, 0, band_pass_filter_voltage1, 0);
        top->hier_block2::connect(band_pass_filter_current1, 0, blocks_multiply_phase1_0, 0);
        top->hier_block2::connect(band_pass_filter_current1, 0, blocks_multiply_phase1_1, 0);
        top->hier_block2::connect(band_pass_filter_current1, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 4); // ????
        top->hier_block2::connect(band_pass_filter_voltage1, 0, blocks_multiply_phase1_2, 0);
        top->hier_block2::connect(band_pass_filter_voltage1, 0, blocks_multiply_phase1_3, 0);
        top->hier_block2::connect(band_pass_filter_voltage1, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 3); // ???
        top->hier_block2::connect(analog_sig_source_phase1_sin, 0, blocks_multiply_phase1_0, 1);
        top->hier_block2::connect(analog_sig_source_phase1_sin, 0, blocks_multiply_phase1_2, 1);
        top->hier_block2::connect(analog_sig_source_phase1_cos, 0, blocks_multiply_phase1_1, 1);
        top->hier_block2::connect(analog_sig_source_phase1_cos, 0, blocks_multiply_phase1_3, 1);
        top->hier_block2::connect(blocks_multiply_phase1_0, 0, low_pass_filter_current1_0, 0);
        top->hier_block2::connect(blocks_multiply_phase1_1, 0, low_pass_filter_current1_1, 0);
        top->hier_block2::connect(blocks_multiply_phase1_2, 0, low_pass_filter_voltage1_0, 0);
        top->hier_block2::connect(blocks_multiply_phase1_3, 0, low_pass_filter_voltage1_1, 0);
        top->hier_block2::connect(low_pass_filter_current1_0, 0, blocks_divide_phase1_0, 0);
        top->hier_block2::connect(low_pass_filter_current1_1, 0, blocks_divide_phase1_0, 1);
        top->hier_block2::connect(low_pass_filter_voltage1_0, 0, blocks_divide_phase1_1, 0);
        top->hier_block2::connect(low_pass_filter_voltage1_1, 0, blocks_divide_phase1_1, 1);
        top->hier_block2::connect(blocks_divide_phase1_0, 0, blocks_transcendental_phase1_0, 0);
        top->hier_block2::connect(blocks_divide_phase1_1, 0, blocks_transcendental_phase1_1, 0);
        top->hier_block2::connect(blocks_transcendental_phase1_0, 0, blocks_sub_phase1, 0);
        top->hier_block2::connect(blocks_transcendental_phase1_1, 0, blocks_sub_phase1, 1);
        top->hier_block2::connect(blocks_sub_phase1, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 5);

        // Phase 2:
        top->hier_block2::connect(analog_sig_source_current2, 0, band_pass_filter_current2, 0);
        top->hier_block2::connect(analog_sig_source_voltage2, 0, band_pass_filter_voltage2, 0);
        top->hier_block2::connect(band_pass_filter_current2, 0, blocks_multiply_phase2_0, 0);
        top->hier_block2::connect(band_pass_filter_current2, 0, blocks_multiply_phase2_1, 0);
        top->hier_block2::connect(band_pass_filter_current2, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 7); // ???
        top->hier_block2::connect(band_pass_filter_voltage2, 0, blocks_multiply_phase2_2, 0);
        top->hier_block2::connect(band_pass_filter_voltage2, 0, blocks_multiply_phase2_3, 0);
        top->hier_block2::connect(band_pass_filter_voltage2, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 6); // ???
        top->hier_block2::connect(analog_sig_source_phase2_sin, 0, blocks_multiply_phase2_0, 1);
        top->hier_block2::connect(analog_sig_source_phase2_sin, 0, blocks_multiply_phase2_2, 1);
        top->hier_block2::connect(analog_sig_source_phase2_cos, 0, blocks_multiply_phase2_1, 1);
        top->hier_block2::connect(analog_sig_source_phase2_cos, 0, blocks_multiply_phase2_3, 1);
        top->hier_block2::connect(blocks_multiply_phase2_3, 0, low_pass_filter_voltage2_1, 0);
        top->hier_block2::connect(blocks_multiply_phase2_2, 0, low_pass_filter_voltage2_0, 0);
        top->hier_block2::connect(blocks_multiply_phase2_1, 0, low_pass_filter_current2_1, 0);
        top->hier_block2::connect(blocks_multiply_phase2_0, 0, low_pass_filter_current2_0, 0);
        top->hier_block2::connect(low_pass_filter_current2_0, 0, blocks_divide_phase2_0, 0);
        top->hier_block2::connect(low_pass_filter_current2_1, 0, blocks_divide_phase2_0, 1);
        top->hier_block2::connect(low_pass_filter_voltage2_0, 0, blocks_divide_phase2_1, 0);
        top->hier_block2::connect(low_pass_filter_voltage2_1, 0, blocks_divide_phase2_1, 1);
        top->hier_block2::connect(blocks_divide_phase2_0, 0, blocks_transcendental_phase2_0, 0);
        top->hier_block2::connect(blocks_divide_phase2_1, 0, blocks_transcendental_phase2_1, 0);
        top->hier_block2::connect(blocks_transcendental_phase2_0, 0, blocks_sub_phase2, 0);
        top->hier_block2::connect(blocks_transcendental_phase2_1, 0, blocks_sub_phase2, 1);
        top->hier_block2::connect(blocks_sub_phase2, 0, pulsed_power_power_calc_mul_ph_ff_0_0, 8);

        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 0, pulsed_power_opencmw_time_sink, 0);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 1, pulsed_power_opencmw_time_sink, 1);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 2, pulsed_power_opencmw_time_sink, 2);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 3, pulsed_power_opencmw_time_sink, 3);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 4, pulsed_power_opencmw_time_sink, 4);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 5, pulsed_power_opencmw_time_sink, 5);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 6, pulsed_power_opencmw_time_sink, 6);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 7, pulsed_power_opencmw_time_sink, 7);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 8, pulsed_power_opencmw_time_sink, 8);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 9, pulsed_power_opencmw_time_sink, 9);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 10, pulsed_power_opencmw_time_sink, 10);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 11, pulsed_power_opencmw_time_sink, 11);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 12, pulsed_power_opencmw_time_sink, 12);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 13, pulsed_power_opencmw_time_sink, 13);
        top->hier_block2::connect(pulsed_power_power_calc_mul_ph_ff_0_0, 14, pulsed_power_opencmw_time_sink, 14);
    }
    ~GRFlowGraphThreePhaseSimulated() { top->stop(); }
    // start gnuradio flowgraph
    void start() { top->start(); }
};

class GRFlowGraphThreePhasePicoscope {
private:
    gr::top_block_sptr top;

public:
    GRFlowGraphThreePhasePicoscope(int noutput_items)
        : top(gr::make_top_block("GNURadio")) {}

    ~GRFlowGraphThreePhasePicoscope() {
        top->stop();
    }
    // start gnuradio flowgraph
    void
    start() {
        top->start();
    }
};
