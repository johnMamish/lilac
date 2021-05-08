# lilac: A Little Audio Compressor
`lilac` is a partial implementation of the [Opus lossy audio codec](https://tools.ietf.org/html/rfc6716) specifically targeted at low-power embedded systems. `lilac` comprises both a partial c implementation and a partial verilog implementation of Opus. `lilac` is capable of coding full-bandwidth audio using the Constrained-Energy Lapped Transform codec.

`lilac` is meant to be retargetable, but until the project is mature, platform specfic code will be used to facilitate development and testing. Default platforms are:
  * iCE40 UP5K FPGA for Verilog
  * ATSAMD21G18A for C