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

#ifndef INCLUDED_SABRSDR_SABR_SOURCE_H
#define INCLUDED_SABRSDR_SABR_SOURCE_H

#include <sabrSDR/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace sabrSDR {

    /*!
     * \brief <+description of block+>
     * \ingroup sabrSDR
     *
     */
    class SABRSDR_API sabr_source : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<sabr_source> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of sabrSDR::sabr_source.
       *
       * To avoid accidental use of raw pointers, sabrSDR::sabr_source's
       * constructor is in a private implementation
       * class. sabrSDR::sabr_source::make is the public interface for
       * creating new instances.
       */
      static sptr make(double frequency, double sampleRate, double gain, int gainMode);

      virtual double set_sample_rate(double rate, int chan = 0) = 0;
      virtual double get_sample_rate(int chan = 0) = 0;

      virtual double set_center_freq(double freq, int chan = 0) = 0;
      virtual double get_center_freq(int chan = 0) = 0;

      virtual double set_bandwidth(double bandwidth, int chan = 0) = 0;
      virtual double get_bandwidth(int chan = 0) = 0;

      virtual double set_gain(double gain, int chan = 0) = 0;
      virtual double get_gain(int chan = 0) = 0;

      virtual int set_gain_mode(int gainMode, int chan = 0) = 0;
      virtual int get_gain_mode(int chan = 0) = 0;
    };
  } // namespace sabrSDR
} // namespace gr

#endif /* INCLUDED_SABRSDR_SABR_SOURCE_H */

