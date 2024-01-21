#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/dsp/io/file_sink.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"
#include "common/widgets/timed_message.h"
#include "common/widgets/constellation.h"

#include "common/widgets/pipeline_selector.h"
#include "core/live_pipeline.h"

#include "tracking/tracking_widget.h"

namespace satdump
{
    class RecorderApplication : public Application
    {
    protected:
        void drawUI();

        uint64_t frequency_hz = 100000000;
        bool show_waterfall = true;
        bool is_started = false, is_recording = false, is_processing = false;

        double xconverter_frequency = 0;

        int selected_fft_size = 0;
        std::vector<int> fft_sizes_lut = {131072, 65536, 16384, 8192, 4096, 2048, 1024};
        int fft_size = 8192; // * 4;
        int fft_rate = 120;
        int waterfall_rate = 60;

        std::vector<colormaps::Map> waterfall_palettes;
        std::string waterfall_palettes_str;
        int selected_waterfall_palette = 0;

        bool dragging_waterfall = false;
        float waterfall_ratio = 0.7;

        bool dragging_panel = false;
        float panel_ratio = 0.2;
        float last_width = -1.0f;

        std::string recorder_filename;
        int select_sample_format = 0;

        widgets::TimedMessage sdr_error = widgets::TimedMessage(ImColor(255, 0, 0), 4);
        widgets::TimedMessage error = widgets::TimedMessage(ImColor(255, 0, 0), 4);

        std::shared_ptr<dsp::DSPSampleSource> source_ptr;
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> decim_ptr;
        std::shared_ptr<dsp::SplitterBlock> splitter;
        std::shared_ptr<dsp::FFTPanBlock> fft;
        std::shared_ptr<dsp::FileSinkBlock> file_sink;
        std::shared_ptr<widgets::FFTPlot> fft_plot;
        std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

        std::vector<dsp::SourceDescriptor> sources;

        int sdr_select_id = -1;
        std::string sdr_select_string;

        bool processing_modules_floating_windows = false;

        bool automated_live_output_dir = false;
        PipelineUISelector pipeline_selector;
        std::unique_ptr<satdump::LivePipeline> live_pipeline;
        std::string pipeline_output_dir;
        nlohmann::json pipeline_params;

        int pipeline_preset_id = 0;

        uint64_t current_samplerate = 1e6;
        int current_decimation = 1;

        // #ifdef BUILD_ZIQ
        int ziq_bit_depth;
        // #endif

        nlohmann::json serialize_config();
        void deserialize_config(nlohmann::json in);
        void try_load_sdr_settings();
        void set_output_sample_format();

        TrackingWidget *tracking_widget = nullptr;
        bool show_tracking = false;
        bool tracking_started_cli = false;

        // Debug
        widgets::ConstellationViewer *constellation_debug = nullptr;

    private: // VFO Stuff
        struct VFOInfo
        {
            ////
            std::string id;
            std::string name;
            double freq;

            //// Live
            int pipeline_id = -1;
            nlohmann::json pipeline_params;
            std::string output_dir;
            std::shared_ptr<ctpl::thread_pool> lpool;
            std::shared_ptr<satdump::LivePipeline> live_pipeline;

            //// Recording
            std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> decim_ptr;
            std::shared_ptr<dsp::FileSinkBlock> file_sink;
        };

        std::mutex vfos_mtx;
        std::vector<VFOInfo> vfo_list;

        void add_vfo_live(std::string id, std::string name, double freq, int vpipeline_id, nlohmann::json vpipeline_params);
        void add_vfo_reco(std::string id, std::string name, double freq, dsp::BasebandType type, int decimation = -1);
        void del_vfo(std::string id);

    private:
        void start();
        void stop();

        void start_processing();
        void stop_processing();

        void start_recording();
        void stop_recording();

        void try_init_tracking_widget();

        uint64_t get_samplerate()
        {
            if (current_decimation > 0)
                return current_samplerate / current_decimation;
            else
                return current_samplerate;
        }

        void set_frequency(uint64_t freq_hz)
        {
            double xconv_freq = frequency_hz = freq_hz;
            if (xconv_freq > xconverter_frequency * 1e6)
                xconv_freq -= xconverter_frequency * 1e6;
            else
                xconv_freq = xconverter_frequency * 1e6 - xconv_freq;

            source_ptr->set_frequency(xconv_freq);
            if (fft_plot)
            {
                fft_plot->frequency = freq_hz;
                if (xconverter_frequency == 0)
                    fft_plot->actual_sdr_freq = -1;
                else
                    fft_plot->actual_sdr_freq = xconv_freq;
            }
        }

    public:
        RecorderApplication();
        ~RecorderApplication();

        void save_settings()
        {
            config::main_cfg["user"]["recorder_state"] = serialize_config();
        }

    public:
        static std::string getID() { return "recorder"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<RecorderApplication>(); }
    };
};