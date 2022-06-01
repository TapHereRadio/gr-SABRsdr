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

#ifndef INCLUDED_SABRSDR_SABR_SINK_IMPL_H
#define INCLUDED_SABRSDR_SABR_SINK_IMPL_H
#define BYTES_PER_SAMPLE 4
#include <sabrSDR/sabr_sink.h>
#include "RadioDevice.h"
#include "ErrorFlags.h"
#include "SpecsEnums.h"
#include <cstdint>
#include <chrono>
using namespace THR;

namespace gr {
	namespace sabrSDR {

		class sabr_sink_impl : public sabr_sink
		{
#define tx1Channel 1
#define txChunkSize 32768
		private:
			RadioDevice sabrDevice;
			int samplesPerChunk;
			std::chrono::high_resolution_clock::time_point t1;
			uint64_t waitTime;

		public:
			sabr_sink_impl(double frequency, double sampleRate, float attenuation);
			~sabr_sink_impl();

			double set_center_freq(double freq, int chan = tx1Channel);
			double get_center_freq(int chan = 1);

			float set_attenuation(float attenuation, int chan = tx1Channel);
			float get_attenuation(int chan = tx1Channel);

			double set_sample_rate(double rate, int chan = tx1Channel);
			double get_sample_rate(int chan = tx1Channel);

			bool start();
			bool stop();

			// Where all the action really happens
			int work(
				int noutput_items,
				gr_vector_const_void_star& input_items,
				gr_vector_void_star& output_items
			);
		};

	} // namespace sabrSDR
} // namespace gr

#endif /* INCLUDED_SABRSDR_SABR_SINK_IMPL_H */

