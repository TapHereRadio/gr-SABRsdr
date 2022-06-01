/* -*- c++ -*- */

#define SABRSDR_API

%include "gnuradio.i"           // the common stuff

//load generated python docstrings
%include "sabrSDR_swig_doc.i"

%{
#include "sabrSDR/sabr_source.h"
#include "sabrSDR/sabr_sink.h"
%}

%include "sabrSDR/sabr_source.h"
GR_SWIG_BLOCK_MAGIC2(sabrSDR, sabr_source);
%include "sabrSDR/sabr_sink.h"
GR_SWIG_BLOCK_MAGIC2(sabrSDR, sabr_sink);
