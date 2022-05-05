#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "common/dsp_sample_source/dsp_sample_source.h"
#include "common/dsp/splitter.h"
#include "common/dsp/fft.h"
#include "common/dsp/file_sink.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

#include "pipeline_selector.h"
#include "live_pipeline.h"

namespace satdump
{
    class RecorderApplication : public Application
    {
    protected:
        void drawUI();

        double frequency_mhz = 100;
        bool is_started = false, is_recording = false, is_processing = false;

        int fft_size = 8192; // * 4;

        std::string recorder_filename;
        int select_sample_format = 0;

        std::shared_ptr<dsp::DSPSampleSource> source_ptr;
        std::shared_ptr<dsp::SplitterBlock> splitter;
        std::shared_ptr<dsp::FFTBlock> fft;
        std::shared_ptr<dsp::FileSinkBlock> file_sink;
        std::shared_ptr<widgets::FFTPlot> fft_plot;
        std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

        std::vector<dsp::SourceDescriptor> sources;

        int sdr_select_id = 0;
        std::string sdr_select_string;

        PipelineUISelector pipeline_selector;
        std::unique_ptr<satdump::LivePipeline> live_pipeline;

//#ifdef BUILD_ZIQ
        int ziq_bit_depth;
//#endif

    public:
        RecorderApplication();
        ~RecorderApplication();

    public:
        static std::string getID() { return "recorder"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<RecorderApplication>(); }
    };
};