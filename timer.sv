module timer(
	input [7:0] clock_reset,
	input clk,
	output [7:0] fps); 
	
	logic [31:0] counter;
	logic count_EN; 
	
	
	always_ff@(posedge clk)
	begin
		if (clock_reset)
		begin
			count_EN <= 1;
			counter <= 0;
			fps <= 0;
		end
		else 
		begin
			if (counter == 25000000)
				begin
					fps <= 1;
					counter <= 0;
					count_EN <= 0;
				end
			else 
				begin
					counter <= counter+1;
				end
		end
	
	end
	
endmodule