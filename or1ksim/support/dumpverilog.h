#define DW 32	/* Data width of memory model generated by dumpverilog in bits */
#define DWQ (DW/8) /* Same as DW but units are bytes */
#define DISWIDTH 25 /* Width of disassembled message in bytes */

#define OR1K_MEM_VERILOG_HEADER(MODNAME, FROMADDR, TOADDR, DISWIDTH) "\n"\
"include \"general.h\"\n\n"\
"`timescale 1ns/100ps\n\n"\
"// Simple dw-wide Sync SRAM with initial content generated by or1ksim.\n"\
"// All control, data in and addr signals are sampled at rising clock edge  \n"\
"// Data out is not registered. Address bits specify dw-word (narrowest \n"\
"// addressed data is not byte but dw-word !). \n"\
"// There are still some bugs in generated output (dump word aligned regions)\n\n"\
"module %s(clk, data, addr, ce, we, disout);\n\n"\
"parameter dw = 32;\n"\
"parameter amin = %d;\n\n"\
"parameter amax = %d;\n\n"\
"input clk;\n"\
"inout [dw-1:0] data;\n"\
"input [31:0] addr;\n"\
"input ce;\n"\
"input we;\n"\
"output [%d:0] disout;\n\n"\
"reg  [%d:0] disout;\n"\
"reg  [dw-1:0] mem [amax:amin];\n"\
"reg  [%d:0] dis [amax:amin];\n"\
"reg  [dw-1:0] dataout;\n"\
"tri  [dw-1:0] data = (ce && ~we) ? dataout : 'bz;\n\n"\
"initial begin\n", MODNAME, FROMADDR, TOADDR, DISWIDTH-1, DISWIDTH-1, DISWIDTH-1

#define OR1K_MEM_VERILOG_FOOTER "\n\
end\n\n\
always @(posedge clk) begin\n\
        if (ce && ~we) begin\n\
                dataout <= #1 mem[addr];\n\
                disout <= #1 dis[addr];\n\
                $display(\"or1k_mem: reading mem[%%0d]:%%h dis: %%0s\", addr, dataout, dis[addr]);\n\
        end else\n\
        if (ce && we) begin\n\
                mem[addr] <= #1 data;\n\
                dis[addr] <= #1 \"(data)\";\n\
                $display(\"or1k_mem: writing mem[%%0d]:%%h dis: %%0s\", addr, mem[addr], dis[addr]);\n\
        end\n\
end\n\n\
endmodule\n"

void dumpverilog(char *verilog_modname, oraddr_t from, oraddr_t to);
void dumphex(oraddr_t from, oraddr_t to);