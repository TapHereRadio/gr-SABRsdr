/* -*- c++ -*- */
/*
 * Copyright 2021 TapHere! Technology.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "sabr_source_impl.h"

using namespace THR;

namespace gr
{
	namespace sabrSDR
	{

		static const int MIN_IN = 0;	// mininum number of input streams
		static const int MAX_IN = 0;	// maximum number of input streams
		static const int MIN_OUT = 1;	// minimum number of output streams
		static const int MAX_OUT = 1;	// maximum number of output streams

		sabr_source::sptr
			sabr_source::make(double frequency, double sampleRate, double gain, int gainMode)
		{
			return gnuradio::get_initial_sptr
			(new sabr_source_impl(frequency, sampleRate, gain, gainMode));
		}

		/*
		 * The private constructor
		 */
		sabr_source_impl::sabr_source_impl(double frequency, double sampleRate, double gain, int gainMode)
			: gr::sync_block("sabr_source",
				gr::io_signature::make(MIN_IN, MAX_IN, sizeof(gr_complex)),
				gr::io_signature::make(MIN_OUT, MAX_OUT, sizeof(gr_complex)))
		{
			ErrorFlags result = sabrDevice.Setup();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cout << "Unable to connect to SABR device!" << std::endl;
				exit(0);
			}
			result = sabrDevice.StartCapture();
			// This should most likely be set based on the desired sample rate for the radio.
			// See how iqStreamSize is set as this is how we do it in other applications.
			// However GNURadio seems to complain about the buffer being to small if we try to increase this later on...
			// Keep in mind the factor of 4 difference between bytes we get from device and number of samples produced.
			set_output_multiple(65536);
			set_max_noutput_items(1048576);
			set_center_freq(frequency);
			set_sample_rate(sampleRate);
			set_gain_mode(gainMode);
			if (gainMode == 0)
			{
				set_gain(gain);
			}
		}

		/*
		 * Our virtual destructor.
		 */
		sabr_source_impl::~sabr_source_impl()
		{
			sabrDevice.StopCapture();
			sabrDevice.CloseDevice();
		}

		int
			sabr_source_impl::work(int noutput_items,
				gr_vector_const_void_star& input_items,
				gr_vector_void_star& output_items)
		{
			gr_complex* out = (gr_complex*)output_items[0];
			uint8_t* rawSamples;
			uint64_t numRawBytes = BYTES_PER_SAMPLE * (uint64_t)noutput_items;
			ErrorFlags result = sabrDevice.ReceiveSamples(rawSamples, numRawBytes);
			int currIndex = 0;
			for (int i = 0; i < noutput_items; i++)
			{
				short currI = (int)rawSamples[currIndex] << 8 | (int)rawSamples[currIndex + 1];
				short currQ = (int)rawSamples[currIndex + 2] << 8 | (int)rawSamples[currIndex + 3];
				currIndex += 4;
				*out++ = gr_complex(currI, currQ);
			}
			delete[] rawSamples;
			// Tell runtime system how many output items we produced.
			return noutput_items;
		}

		bool sabr_source_impl::start()
		{
			ErrorFlags result = sabrDevice.StartCapture();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cerr << "Failed to start RX streaming (" << result << ")" << std::endl;
				return false;
			}
			return true;
		}

		bool sabr_source_impl::stop()
		{
			ErrorFlags result = sabrDevice.StopCapture();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cerr << "Failed to stop RX streaming (" << result << ")" << std::endl;
				return false;
			}
			return true;
		}

		double sabr_source_impl::get_sample_rate(int chan)
		{
			uint64_t receivedSampleRate;
			ErrorFlags result = sabrDevice.GetSampleRate(chan, receivedSampleRate);
			return (double)receivedSampleRate;
		}

		double sabr_source_impl::set_sample_rate(double rate, int chan)
		{
			ErrorFlags result = sabrDevice.SetSampleRate(chan, (uint64_t)rate);
			return get_sample_rate(chan);
		}

		double sabr_source_impl::get_center_freq(int chan)
		{
			uint64_t receivedFrequency;
			ErrorFlags result = sabrDevice.GetLOFrequency(chan, receivedFrequency);
			return (double)receivedFrequency;
		}

		double sabr_source_impl::set_center_freq(double freq, int chan)
		{
			ErrorFlags result = sabrDevice.SetLOFrequency(chan, (uint64_t)freq);
			return get_center_freq(chan);
		}

		int sabr_source_impl::set_gain_mode(int gainMode, int chan)
		{
			ErrorFlags result = sabrDevice.SetGainMode(chan, (RadioGainMode)gainMode);
			return get_gain_mode(chan);
		}

		int sabr_source_impl::get_gain_mode(int chan)
		{
			RadioGainMode gainMode;
			ErrorFlags result = sabrDevice.GetGainMode(chan, gainMode);
			return (int)gainMode;
		}

		double sabr_source_impl::get_gain(int chan)
		{
			int gain;
			ErrorFlags result = sabrDevice.GetGain(chan, gain);
			return (double)gain;
		}

		double sabr_source_impl::set_gain(double gain, int chan)
		{
			ErrorFlags result = sabrDevice.SetGain(chan, (int)gain);
			return get_gain(chan);
		}

		double sabr_source_impl::set_bandwidth(double bandwidth, int chan)
		{
			ErrorFlags result = sabrDevice.SetComplexBandwidth(chan, (uint64_t)bandwidth);
			return get_bandwidth(chan);
		}

		double sabr_source_impl::get_bandwidth(int chan)
		{
			uint64_t bandwidth;
			ErrorFlags result = sabrDevice.GetComplexBandwidth(chan, bandwidth);
			return (double)bandwidth;
		}

	} /* namespace sabrSDR */
} /* namespace gr */

