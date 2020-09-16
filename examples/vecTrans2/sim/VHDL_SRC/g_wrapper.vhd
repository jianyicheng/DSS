--=================================================
--Dynamic & Static Scheduling
--Component Name: call_0(g) - (2, 15)
--09/16/20 14:13:15
--=================================================
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;
use ieee.math_real.all;
entity ss_g is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer; LATENCY: integer; II: integer);
port(
	dataInArray : in data_array (INPUTS-1 downto 0)(DATA_SIZE_IN-1 downto 0);
	pValidArray : IN std_logic_vector(INPUTS-1 downto 0);
	readyArray : OUT std_logic_vector(INPUTS-1 downto 0);
	dataOutArray : out data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);
	nReadyArray: in std_logic_vector(OUTPUTS-1 downto 0);
	validArray: out std_logic_vector(OUTPUTS-1 downto 0);
	clk, rst: in std_logic
);
end entity;

architecture arch of ss_g is

component g is
port (
    ap_clk : IN STD_LOGIC;
    ap_rst : IN STD_LOGIC;
    ap_start : IN STD_LOGIC;
    ap_done : OUT STD_LOGIC;
    ap_idle : OUT STD_LOGIC;
    ap_ready : OUT STD_LOGIC;
    ap_ce : IN STD_LOGIC;
    d : IN STD_LOGIC_VECTOR (31 downto 0);
    ap_return : OUT STD_LOGIC_VECTOR (31 downto 0) );
end component;

signal ap_clk : STD_LOGIC;
signal ap_rst : STD_LOGIC;
signal ap_start : STD_LOGIC;
signal ap_done : STD_LOGIC;
signal ap_idle : STD_LOGIC;
signal ap_ready : STD_LOGIC;
signal ap_ce : STD_LOGIC;
signal d : STD_LOGIC_VECTOR (31 downto 0);
signal ap_return : STD_LOGIC_VECTOR (31 downto 0);
signal preValid : STD_LOGIC;
signal nextReady : STD_LOGIC;
signal oehb_ready: std_logic_vector(OUTPUTS-1 downto 0);
signal oehb_datain: data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);
signal oehb_dataout: data_array (OUTPUTS-1 downto 0)(DATA_SIZE_OUT-1 downto 0);
constant depth: integer := integer(ceil(real(LATENCY)/real(II)));
signal shift_reg: std_logic_vector(depth-1 downto 0);
signal valid:std_logic;
signal ready_buf:std_logic;
signal buf_in:data_array (INPUTS-1 downto 0)(DATA_SIZE_IN-1 downto 0);

begin

process(clk)
begin
	if rising_edge(clk) then
		ready_buf <= ap_ready;
	end if;
end process;
	process(clk, rst)
	begin
		if rst = '1' then
			shift_reg <= (others => '0');
		elsif rising_edge(clk) then
			if ready_buf = '1' and ap_ce = '1' then
				shift_reg <= shift_reg(depth-2 downto 0) & preValid;
			else
				shift_reg <= shift_reg;
			end if;
		end if;
	end process;
	valid <= shift_reg(depth-1) and ap_done;
	ap_start <= '1';
	ap_clk <= clk;
	ap_rst <= rst;
	dataOutArray(0) <= oehb_dataOut(0);
	oehb_datain(0) <= ap_return;
	ap_ce <= not (validArray(0) and not nextReady);
	nextReady <= oehb_ready(0);
ob: entity work.OEHB(arch) generic map (1, 1, DATA_SIZE_OUT, DATA_SIZE_OUT)
port map (
	clk => clk, 
	rst => rst, 
	pValidArray(0)  => valid, 
	nReadyArray(0) => nReadyArray(0),
	validArray(0) => validArray(0), 
	readyArray(0) => oehb_ready(0),   
	dataInArray(0) => oehb_datain(0),
	dataOutArray(0) => oehb_dataOut(0)
);
	preValid <= pValidArray(0);
process(clk, rst)
begin
	if rst = '1' then
		buf_in(0) <= (others => '0');
	elsif rising_edge(clk) then
		if pValidArray(0) = '1' then
			buf_in(0) <= dataInArray(0);
		end if;
	end if;
end process;
d <= dataInArray(0) when pValidArray(0) = '1' else buf_in(0);
readyArray(0) <= (not pValidArray(0)) or (preValid and ready_buf);

func: g
port map (
	ap_clk => ap_clk,
	ap_rst => ap_rst,
	ap_start => ap_start,
	ap_done => ap_done,
	ap_idle => ap_idle,
	ap_ready => ap_ready,
	ap_ce => ap_ce,
	d => d,
	ap_return => ap_return
);

end architecture;

--========================END=====================
