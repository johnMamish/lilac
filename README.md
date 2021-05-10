# lilac: A Little Audio Compressor
`lilac` is a partial implementation of the [Opus lossy audio codec](https://tools.ietf.org/html/rfc6716) specifically targeted at low-power embedded systems. `lilac` comprises both a partial c implementation and a partial verilog implementation of Opus. `lilac` is capable of coding full-bandwidth audio using the Constrained-Energy Lapped Transform codec.

`lilac` is meant to be retargetable, but until the project is mature, platform specfic code will be used to facilitate development and testing. Default platforms are:
  * iCE40 UP5K FPGA for Verilog
  * ATSAMD21G18A for C


# Features
----------------------------------------------------------------
So that I can get this project off the ground, I'm starting with a very restricted version of Opus that has many of the features unimplemented. The unimplemented features are all optional, but result in better audio quality at the given bitrate. As lilac matures, more and more of these features will be added.

##### v0.1 Supports
  * Encoding using only the CELT algorithm
  * 10 millisecond frames
  * Coding 48ksps sources
  * Mono audio only
  * Full-Bandwidth coding at 64kbps coding rate

##### Features not implemented in v0.1, but desired for future versions
  * CELT-based NB, WB, SWB, and FB encoding
  * 2.5, 5, 10, and 20ms frame sizes
  * improved transient coding
  * VBR (Variable bit-rate)
  * Variable time-frequency resolution
  * Inter-frame predictive coding
  * pitch pre-filter

##### Features that aren't planned
  * per-band variable time-frequency resolution
  * SILK-based encoding
  * stereo or multi-channel audio
