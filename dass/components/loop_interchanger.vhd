 -- loop_interchanger
 library IEEE;
 use IEEE.std_logic_1164.all;
 use IEEE.numeric_std.all;
 use work.customTypes.all;
 
 entity loop_interchanger is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer; DEPTH : integer);
 port(
     dataInArray : IN std_logic_vector(INPUTS*DATA_SIZE_IN-1 downto 0);
     pValidArray : IN std_logic_vector(INPUTS-1 downto 0);
     readyArray : INOUT std_logic_vector(INPUTS-1 downto 0);
     dataOutArray : INOUT std_logic_vector(OUTPUTS*DATA_SIZE_OUT-1 downto 0);
     nReadyArray: IN std_logic_vector(OUTPUTS-1 downto 0);
     validArray : INOUT std_logic_vector(INPUTS-1 downto 0);
     clk, rst: IN std_logic
 );
 end entity;
 
 architecture arch of loop_interchanger is
 
 signal state_counter : integer range 0 to DEPTH;
 signal new_entry : std_logic;
 signal new_exit : std_logic;

 signal counter_full : std_logic;
 signal enable : std_logic;

 begin

    new_entry <= pValidArray(0) and nReadyArray(0);
    new_exit <= pValidArray(1) and '1'; -- nReadyArray(1);
    counter_full <= '1' when state_counter = DEPTH else '0';

    -- state_counter : count the number of tokens flowing in the loop
    process(rst, clk)
    begin
        if rst = '1' then
            state_counter <= 0;
        elsif rising_edge(clk) then
            if new_entry = '1' and new_exit = '0' and counter_full = '0' then
                state_counter <= state_counter + 1;
            elsif new_exit = '1' and new_entry = '0' then
                state_counter <= state_counter - 1;
            else
                state_counter <= state_counter;
            end if;
        end if;
    end process;

    readyArray(1) <= '1';
    enable <= (not counter_full or new_exit) and new_entry and nReadyArray(1);

    readyArray(0) <= not pValidArray(0) or enable;
    validArray(1) <= pValidArray(0) and enable;
    validArray(0) <= pValidArray(0) and enable;

 end architecture;

 -- buffer_array
 library IEEE;
 use IEEE.std_logic_1164.all;
 use IEEE.numeric_std.all;
 use work.customTypes.all;

 entity buffer_array is generic(INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer; DEPTH : integer);
 port(
     dataInArray : IN std_logic_vector(INPUTS*DATA_SIZE_IN-1 downto 0);
     pValidArray : IN std_logic_vector(INPUTS-1 downto 0);
     readyArray : INOUT std_logic_vector(INPUTS-1 downto 0);
     dataOutArray : INOUT std_logic_vector(OUTPUTS*DATA_SIZE_OUT-1 downto 0);
     nReadyArray: IN std_logic_vector(OUTPUTS-1 downto 0);
     validArray : INOUT std_logic_vector(INPUTS-1 downto 0);
     clk, rst: IN std_logic
 );
 end entity;
 
 architecture arch of buffer_array is
 
 signal data_array : std_logic_vector(DATA_SIZE_IN*(DEPTH+1)-1 downto 0);
 signal valid_array : std_logic_vector(DEPTH downto 0);
 signal ready_array : std_logic_vector(DEPTH downto 0);

 begin

    data_array(DATA_SIZE_IN-1 downto 0) <= dataInArray;
    dataOutArray <= data_array(DATA_SIZE_IN*(DEPTH+1)-1 downto DATA_SIZE_IN*DEPTH);
    validArray(0) <= valid_array(DEPTH);
    valid_array(0) <= pValidArray(0);
    readyArray(0) <= ready_array(DEPTH);
    ready_array(0) <= nReadyArray(0);

      ba: 
      for I in 0 to DEPTH-1 generate
         buff : entity work.elasticBuffer(arch) generic map (INPUTS, OUTPUTS, DATA_SIZE_IN, DATA_SIZE_OUT)
           port map (
            clk => clk,
            rst => rst,
            dataInArray(DATA_SIZE_IN-1 downto 0) => data_array(DATA_SIZE_IN*(I+1)-1 downto DATA_SIZE_IN*I),
            pValidArray(0) => valid_array(I),
            readyArray(0) => ready_array(I+1),
            nReadyArray(0) => ready_array(I),
            validArray(0) => valid_array(I+1),
            dataOutArray(DATA_SIZE_OUT-1 downto 0) => data_array(DATA_SIZE_OUT*(I+2)-1 downto DATA_SIZE_OUT*(I+1))
           );
      end generate ba;
 end architecture;
