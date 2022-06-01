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
#include "sabr_sink_impl.h"

using namespace THR;

namespace gr
{
	namespace sabrSDR
	{

		sabr_sink::sptr
			sabr_sink::make(double frequency, double sampleRate, float attenuation)
		{
			return gnuradio::get_initial_sptr
			(new sabr_sink_impl(frequency, sampleRate, attenuation));
		}

		// Number of input streams
		static const int MIN_IN = 1;
		static const int MAX_IN = 1;
		// Number of output streams  
		static const int MIN_OUT = 0;
		static const int MAX_OUT = 0;

		/*
		 * The private constructor
		 */
		sabr_sink_impl::sabr_sink_impl(double frequency, double sampleRate, float attenuation)
			: gr::sync_block("sabr_sink",
				gr::io_signature::make(MIN_IN, MAX_IN, sizeof(gr_complex)),
				gr::io_signature::make(MIN_OUT, MAX_OUT, sizeof(gr_complex)))
		{
			ErrorFlags result = sabrDevice.Setup();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cerr << "Unable to connect to SABR device!" << std::endl;
				exit(0);
			}
			samplesPerChunk = txChunkSize / BYTES_PER_SAMPLE;
			t1 = std::chrono::high_resolution_clock::now();
			set_output_multiple(samplesPerChunk);
			start();
			set_center_freq(frequency);
			set_sample_rate(sampleRate);
			set_attenuation(attenuation);
		}

		/*
		 * Our virtual destructor.
		 */
		sabr_sink_impl::~sabr_sink_impl()
		{
			stop();
			sabrDevice.CloseDevice();
		}

		int
			sabr_sink_impl::work(int noutput_items,
				gr_vector_const_void_star& input_items,
				gr_vector_void_star& output_items)
		{
			int numSamplesIn = noutput_items;
			int numPipeTransfers = numSamplesIn / samplesPerChunk;

			const gr_complex* in = (const gr_complex*)input_items[0];

			//Convert number of input items into bytes then send to the radio
			// Currently assumes that input is scaled properly before hand
			int sampleIndex = 0;
			short currI;
			short currQ;
			uint8_t* sampleBytes = new uint8_t[txChunkSize];
			for (int i = 0; i < numPipeTransfers; i++)
			{
				int byteIndex = 0;
				for (int j = 0; j < samplesPerChunk; j++)
				{
					// Grab the I and Q sample from each complex value
					currI = (short)real(in[sampleIndex]);
					currQ = (short)imag(in[sampleIndex]);
					sampleIndex++;
					// Interleave I and Q samples
					sampleBytes[byteIndex] = (uint8_t)(currI >> 8);
					sampleBytes[byteIndex + 1] = (uint8_t)currI;
					sampleBytes[byteIndex + 2] = (uint8_t)(currQ >> 8);
					sampleBytes[byteIndex + 3] = (uint8_t)currQ;
					byteIndex += 4;
				}
				
				// We want to ensure that samples aren't sent out too fast. They should be delivered as close to the sample rate as possible.
				while (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-t1).count() < waitTime)
				{
				}
				ErrorFlags result = sabrDevice.TransmitSamples(sampleBytes, txChunkSize);
				// Restart timer
				t1 = std::chrono::high_resolution_clock::now();
			}
			// Tell runtime system how many input items we consumed
			consume_each(numSamplesIn);
			delete[] sampleBytes;
			return 0;
		}

		bool sabr_sink_impl::start()
		{
			ErrorFlags result = sabrDevice.StartTransmit();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cerr << "Failed to start TX streaming (" << result << ")" << std::endl;
				return false;
			}
			return true;
		}

		bool sabr_sink_impl::stop()
		{
			ErrorFlags result = sabrDevice.StopTransmit();
			if (ERROR_FLAGS_FAILURE(result))
			{
				std::cerr << "Failed to stop TX streaming (" << result << ")" << std::endl;
				return false;
			}
			return true;
		}

		double sabr_sink_impl::get_sample_rate(int chan)
		{
			uint64_t receivedSampleRate;
			ErrorFlags result = sabrDevice.GetSampleRate(chan, receivedSampleRate);
			return (double)receivedSampleRate;
		}

		double sabr_sink_impl::set_sample_rate(double rate, int chan)
		{
			ErrorFlags result = sabrDevice.SetSampleRate(chan, (uint64_t)rate);
			waitTime = (uint64_t)((double)samplesPerChunk / rate * 1000000000); 
			return get_sample_rate(chan);
		}

		double sabr_sink_impl::get_center_freq(int chan)
		{
			uint64_t receivedFrequency;
			ErrorFlags result = sabrDevice.GetLOFrequency(chan, receivedFrequency);
			return (double)receivedFrequency;
		}

		double sabr_sink_impl::set_center_freq(double freq, int chan)
		{
			ErrorFlags result = sabrDevice.SetLOFrequency(chan, (uint64_t)freq);
			return get_center_freq(chan);
		}

		float sabr_sink_impl::set_attenuation(float attenuation, int chan)
		{
			ErrorFlags result = sabrDevice.SetTransmitAttenuation(chan, attenuation);
			return get_attenuation(chan);
		}

		float sabr_sink_impl::get_attenuation(int chan)
		{
			float receivedAttenuation;
			ErrorFlags result = sabrDevice.GetTransmitAttenuation(chan, receivedAttenuation);
			return receivedAttenuation;
		}

	} /* namespace sabrSDR */
} /* namespace gr */

