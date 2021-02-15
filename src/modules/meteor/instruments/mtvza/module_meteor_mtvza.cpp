#include "module_meteor_mtvza.h"
#include <fstream>
#include "modules/meteor/simpledeframer.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>
#include "mtvza_reader.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace meteor
{
    namespace mtvza
    {
        METEORMTVZADecoderModule::METEORMTVZADecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void METEORMTVZADecoderModule::process()
        {
            size_t filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MTVZA";

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1024];

            std::vector<uint8_t> mtvzaData;

            // MSU-MR data
            SimpleDeframer<uint64_t, 32, 248 * 8, 0xFB386A45> mtvzarDefra;

            MTVZAReader reader;

            logger->info("Demultiplexing and deframing...");

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1024);

                // Extract MTVZA
                mtvzaData.insert(mtvzaData.end(), &buffer[15 - 1], &buffer[15 - 1] + 8);
                mtvzaData.insert(mtvzaData.end(), &buffer[271 - 1], &buffer[271 - 1] + 8);
                mtvzaData.insert(mtvzaData.end(), &buffer[527 - 1], &buffer[527 - 1] + 8);
                mtvzaData.insert(mtvzaData.end(), &buffer[783 - 1], &buffer[783 - 1] + 8);

                // Deframe
                std::vector<std::vector<uint8_t>> mtvza_frames = mtvzarDefra.work(mtvzaData);

                for (std::vector<uint8_t> frame : mtvza_frames)
                    reader.work(frame.data());

                mtvzaData.clear();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            data_in.close();

            logger->info("MTVZA Lines          : " + std::to_string(reader.lines));

            logger->info("Writing images.... (Can take a while)");

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Global Composite...");
            int all_width_count = 20;
            int all_height_count = 6;
            cimg_library::CImg<unsigned short> imageAll(26 * 5 * all_width_count, reader.getChannel(0).height() * all_height_count, 1, 1);
            {
                int height = reader.getChannel(0).height();

                for (int row = 0; row < all_height_count; row++)
                {
                    for (int column = 0; column < all_width_count; column++)
                    {
                        if (row * all_width_count + column >= 8461)
                            break;

                        imageAll.draw_image(26 * 5 * column, height * row, 0, 0, reader.getChannel(row * all_width_count + column));
                    }
                }
            }
            WRITE_IMAGE(imageAll, directory + "/MTVZA-ALL.png");
        }

        std::string METEORMTVZADecoderModule::getID()
        {
            return "meteor_mtvza";
        }

        std::vector<std::string> METEORMTVZADecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> METEORMTVZADecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
        {
            return std::make_shared<METEORMTVZADecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace mtvza
} // namespace meteor