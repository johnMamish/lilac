`timescale 1ns/100ps

module serialmic(output    spi_cs,
                 output    uart_tx,
                 output    led,

                 input osc_12m,

                 // i2s connection
                 input     i2s_data,
                 output    i2s_bck,
                 output    i2s_lrck,

                 output [5:0] gpio);
    assign spi_cs = 1'b1;

    // "resetter" is defined in ../common/util.v
    wire reset;
    resetter r(.clock(osc_12m),
               .reset(reset));

    ////////////////////////////////////////////////
    // I2S
    // divide clock by 4 for sample rate of.... 46875. gag.
    wire [31:0] i2s_data_out_0, i2s_data_out_1;
    wire data_valid;
    i2s_controller i2sc(.clock(osc_12m),
                        .reset(reset),

                        .data_valid(data_valid),
                        .data_out_0(i2s_data_out_0),
                        .data_out_1(),

                        .i2s_data(i2s_data),
                        .bck(i2s_bck),
                        .lrck(i2s_lrck));
    defparam i2sc.bits_per_word = 32;
    defparam i2sc.bck_divisor = 4;

    ////////////////////////////////////////////////
    // UART
    reg [7:0] data;
    wire uart_busy;
    uart_tx_kiss ut(.clock(osc_12m),
                    .reset(reset),
                    .data_valid(data_valid),
                    //.data(i2s_data_out_0[31:24]),
                    .data({i2s_data_out_0[31], i2s_data_out_0[28:22]}),
                    .uart_tx(uart_tx),
                    .uart_busy(uart_busy));
    defparam ut.baud_divisor = 6;

    assign gpio = {1'bz, data_valid, i2s_lrck, i2s_bck, i2s_data, uart_tx};
endmodule
