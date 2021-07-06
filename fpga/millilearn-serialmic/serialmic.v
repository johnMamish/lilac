`timescale 1ns/100ps

module serialmic(output    spi_cs,
                 output    uart_tx,
                 output    led,

                 input osc_12m);
    assign spi_cs = 1'b1;

    // "resetter" is defined in ../common/util.v
    wire reset;
    resetter r(.clock(osc_12m),
               .reset(reset));

    // "divide_by_n" is defined in ../common/util.v
    wire baud_clock;
    divide_by_n #(.N(6)) div(.clk(osc_12m),
                               .reset(reset),
                               .out(baud_clock));

    reg [7:0] data;
    reg data_valid;
    wire uart_busy;
    uart_tx ut(.clock(osc_12m),
               .reset(reset),
               .baud_clock(baud_clock),
               .data_valid(data_valid),
               .data(data),
               .uart_tx(uart_tx),
               .uart_busy(uart_busy));

    localparam [7:0] wave_period = 8'd100;
    localparam [7:0] osc_ticks_per_samp = 8'd250;
    reg [15:0] osccnt;
    reg [15:0] wavecnt;

    always @(posedge osc_12m) begin
        if (reset) begin
            osccnt <= 'h0;
            wavecnt <= 'h0;
        end else begin
            if (osccnt == (osc_ticks_per_samp - 1)) begin
                osccnt <= 'h0;
                data_valid <= 'b1;
                if (data == (wave_period - 1)) begin
                    data <= 0;
                end else begin
                    data <= data + 1;
                end
            end else begin
                osccnt <= osccnt + 'h1;
                data_valid <= 'b0;
                data <= data;
            end
        end
    end
endmodule
