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

#ifndef INCLUDED_SABRSDR_SABR_SOURCE_IMPL_H
#define INCLUDED_SABRSDR_SABR_SOURCE_IMPL_H
#define BYTES_PER_SAMPLE 4
#include <sabrSDR/sabr_source.h>
#include "RadioDevice.h"
#include "ErrorFlags.h"
#include "SpecsEnums.h"
#include <cstdint>
using namespace THR;

namespace gr {
	namespace sabrSDR {

		class sabr_source_impl : public sabr_source
		{
		private:
			RadioDevice sabrDevice;
			uint32_t rawReceiveLength;

		public:
			sabr_source_impl(double frequency, double sampleRate, double gain, int gainMode);
			~sabr_source_impl();

			double set_sample_rate(double rate, int chan = 0);
			double get_sample_rate(int chan = 0);

			double set_center_freq(double freq, int chan = 0);
			double get_center_freq(int chan = 0);

			int set_gain_mode(int gainMode, int chan = 0);
			int get_gain_mode(int chan = 0);

			double set_gain(double gain, int chan = 0);
			double get_gain(int chan = 0);

			double set_bandwidth(double bandwidth, int chan = 0);
			double get_bandwidth(int chan = 0);

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

#endif /* INCLUDED_SABRSDR_SABR_SOURCE_IMPL_H */

