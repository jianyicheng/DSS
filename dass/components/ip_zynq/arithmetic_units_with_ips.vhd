-- Vitis HLS IP Wrappers for DS Components
-- Version: 2020.2
-- Target: xc7z020clg484-1

-----------------------------------------------------------------------
-- dass join, version 0.0
-----------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity dass_join is generic (SIZE : integer);
port (
    pValidArray     : in std_logic_vector(SIZE-1 downto 0);
    nReady          : in std_logic;
    valid           : inout std_logic;
    readyArray      : inout std_logic_vector(SIZE-1 downto 0));   
end dass_join;

architecture arch of dass_join is
signal allPValid : std_logic;
    
begin
    
    allPValidAndGate : entity work.andN generic map(SIZE)
            port map(   pValidArray,
                        allPValid);
    
    valid <= allPValid and nReady;
    
    readyArray <= not pValidArray when valid = '0' else (others => '1');

    -- process (pValidArray, nReady)
    -- begin
    --     for i in 0 to SIZE-1 loop
    --         if pValidArray(I) = '0' then
    --             readyArray(I) <= '1';
    --         else
    --             readyArray(I) <= allPValid and nReady;
    --         end if;
    --     end loop;
    -- end process;
          
end arch;

-----------------------------------------------------------------------
-- join_op, version 0.0
-----------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity join_op is
generic (
 INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 1; DATA_SIZE_OUT: integer := 1
);
port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(INPUTS-1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(INPUTS-1 downto 0);
        dataInArray : in std_logic_vector(INPUTS*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0));
end join_op;

architecture arch of join_op is
begin
    join_write_temp:   entity work.dass_join(arch) generic map(INPUTS)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      validArray(0),         --valid          
                      readyArray);   --readyarray 
    dataOutArray <= dataInArray(DATA_SIZE_IN-1 downto 0);
end arch;

-----------------------------------------------------------------------
-- int div, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity int_div is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of int_div is

    component intop_sdiv_32ns_32ns_32_36_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    signal remd : STD_LOGIC_VECTOR(31 downto 0);
    constant st : integer := 36;
    signal q : std_logic_vector(st-2 downto 0);

begin
    intop_sdiv_32ns_32ns_32_36_1_U1 :  component intop_sdiv_32ns_32ns_32_36_1
    generic map (
        ID => 1,
        NUM_STAGE => 36,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        dout => dout);

    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity div_op is
Generic (
 INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0);
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of div_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);
signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     divide: entity work.int_div (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nready_tmp,         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

dataOutArray <= alu_out;
validArray(0) <= alu_valid;
nready_tmp <= nReadyArray(0);

end architecture;

-----------------------------------------------------------------------
-- int mod, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity int_mod is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of int_mod is

    component intop_srem_32ns_32ns_32_36_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    constant st : integer := 36;
    signal q : std_logic_vector(st-2 downto 0);
    signal dummy : STD_LOGIC_VECTOR(31 downto 0);

begin
    intop_srem_32ns_32ns_32_36_1_U1 :  component intop_srem_32ns_32ns_32_36_1
    generic map (
        ID => 1,
        NUM_STAGE => 36,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        dout => dout);

    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity mod_op is
Generic (
 INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0);
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of mod_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);
signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     modulo: entity work.int_mod (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nready_tmp,         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

dataOutArray <= alu_out;
validArray(0) <= alu_valid;
nready_tmp <= nReadyArray(0);

end architecture;

----------------------------------------------------------------------- 
-- double add, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_doubleAdd is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of ALU_doubleAdd is

    component dop_dadd_64ns_64ns_64_7_full_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (63 downto 0);
        din1 : IN STD_LOGIC_VECTOR (63 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    constant st : integer := 7;
    signal q : std_logic_vector(st-2 downto 0);

begin

    dop_dadd_64ns_64ns_64_7_full_dsp_1_U1 : component dop_dadd_64ns_64ns_64_7_full_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => st,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 64)
    port map (
        clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);

    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dadd_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 64; DATA_SIZE_OUT: integer := 64
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dadd_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     add: entity work.ALU_doubleAdd (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0) <= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- double sub, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_doubleSub is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of ALU_doubleSub is

    component dop_dsub_64ns_64ns_64_7_full_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (63 downto 0);
        din1 : IN STD_LOGIC_VECTOR (63 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    constant st : integer := 7;
    signal q : std_logic_vector(st-2 downto 0);

begin

    dop_dsub_64ns_64ns_64_7_full_dsp_1_U1 : component dop_dsub_64ns_64ns_64_7_full_dsp_1
    generic map (
        ID => 2,
        NUM_STAGE => st,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 64)
    port map (
        clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);

    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dsub_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 64; DATA_SIZE_OUT: integer := 64
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dsub_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     sub: entity work.ALU_doubleSub (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0) <= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- double mul, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity doubleMul is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of doubleMul is

component dop_dmul_64ns_64ns_64_7_max_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (63 downto 0);
        din1 : IN STD_LOGIC_VECTOR (63 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    constant st : integer := 7;
    signal q : std_logic_vector(st-2 downto 0);

begin

 dop_dmul_64ns_64ns_64_7_max_dsp_1_U1 : component dop_dmul_64ns_64ns_64_7_max_dsp_1
    generic map (
        ID => 3,
        NUM_STAGE => st,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 64)
    port map (
        clk =>  clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dmul_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 64; DATA_SIZE_OUT: integer := 64
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dmul_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     multiply: entity work.doubleMul (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0)<= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- double div, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity doubleDiv is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of doubleDiv is

component dop_ddiv_64ns_64ns_64_59_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (63 downto 0);
        din1 : IN STD_LOGIC_VECTOR (63 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    constant st : integer := 59;
    signal q : std_logic_vector(st-2 downto 0);

begin

 dop_ddiv_64ns_64ns_64_59_no_dsp_1_U1 : component dop_ddiv_64ns_64ns_64_59_no_dsp_1
    generic map (
        ID => 4,
        NUM_STAGE => st,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 64)
    port map (
        clk =>  clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ddiv_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 64; DATA_SIZE_OUT: integer := 64
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of ddiv_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     divide: entity work.doubleDiv (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0)<= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- float add, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatAdd is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatAdd is

    component fop_fadd_32ns_32ns_32_5_full_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : OUT STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    constant st : integer := 5;
    signal q : std_logic_vector(st-2 downto 0);
    signal ce : std_logic;

begin

    fop_fadd_32ns_32ns_32_5_full_dsp_1_U1 : component fop_fadd_32ns_32ns_32_5_full_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => st,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => ce,
        dout => dout);

    ready<= ce;
    ce <= nReady or not q(st-2);

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (ce='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fadd_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture no_buff of fadd_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     add: entity work.ALU_floatAdd (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0) <= alu_valid;

end no_buff;

architecture arch of fadd_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

signal buff_ready : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     add: entity work.ALU_floatAdd (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     buff_ready,         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    add_buff: entity work.elasticBuffer(arch) generic map (1,1,DATA_SIZE_IN,DATA_SIZE_OUT)
    port map (
            clk => clk,
            rst => rst,
            dataInArray(DATA_SIZE_IN-1 downto 0) => alu_out,
            pValidArray(0) => alu_valid,
            readyArray(0) => buff_ready,
            nReadyArray(0) => nReadyArray(0),
            validArray(0) => validArray(0),
            dataOutArray => dataOutArray
    );

end arch;

----------------------------------------------------------------------- 
-- float sub, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatSub is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatSub is

    component fop_fsub_32ns_32ns_32_5_full_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    constant st : integer := 5;
    signal q : std_logic_vector(st-2 downto 0);

begin

    fop_fsub_32ns_32ns_32_5_full_dsp_1_U1 : component fop_fsub_32ns_32ns_32_5_full_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => st,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk => clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);

    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fsub_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fsub_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     sub: entity work.ALU_floatSub (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0) <= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- float mul, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity floatMul is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of floatMul is

component fop_fmul_32ns_32ns_32_4_max_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    constant st : integer := 4;
    signal q : std_logic_vector(st-2 downto 0);
    signal ce : std_logic;

begin

fop_fmul_32ns_32ns_32_4_max_dsp_1_U1 : component fop_fmul_32ns_32ns_32_4_max_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => st,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk =>  clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => ce,
        dout => dout);

    ready<= ce;
    ce <= nReady or not q(st-2);

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (ce='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);
end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fmul_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fmul_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     multiply: entity work.floatMul (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0)<= alu_valid;

end architecture;

----------------------------------------------------------------------- 
-- float div, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity floatDiv is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of floatDiv is

component fop_fdiv_32ns_32ns_32_16_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        dout : inout STD_LOGIC_VECTOR (31 downto 0) );
    end component;

    constant st : integer := 16;
    signal q : std_logic_vector(st-2 downto 0);

begin

 fop_fdiv_32ns_32ns_32_16_no_dsp_1_U1 : component fop_fdiv_32ns_32ns_32_16_no_dsp_1
    generic map (
        ID => 4,
        NUM_STAGE => st,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 32)
    port map (
        clk =>  clk,
        reset => reset,
        din0 => din0,
        din1 => din1,
        ce => nready,
        dout => dout);


    ready<= nready;

    process (clk) is
    begin
       if rising_edge(clk) then  
         if (reset = '1') then
             q <= (others => '0');
             q(0) <= pvalid;
         elsif (nready='1') then
             q(0) <= pvalid;
             q(st-2 downto 1) <= q(st-3 downto 0);
          end if;
       end if;
    end process;
    valid <= q(st-2);

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fdiv_op is
Generic (
  INPUTS: integer := 2; OUTPUTS: integer := 1; DATA_SIZE_IN: integer := 32; DATA_SIZE_OUT: integer := 32
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
        dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : inout std_logic_vector(0 downto 0);
        readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fdiv_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     divide: entity work.floatDiv (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     alu_out);  --dout

    dataOutArray <= alu_out;
    validArray(0)<= alu_valid;

end architecture;


-----------------------------------------------------------------------
-- fcmp alu, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_floatCmp is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(0 DOWNTO 0));
end entity;

architecture oeq of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;

    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00001";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

architecture ogt of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;

    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00010";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

architecture oge of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00011";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

architecture olt of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00100";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

architecture ole of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00101";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

architecture one of ALU_floatCmp is

    component fop_fcmp_32ns_32ns_1_2_no_dsp_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR (31 downto 0);
        din1 : IN STD_LOGIC_VECTOR (31 downto 0);
        ce : IN STD_LOGIC;
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    constant opcode : STD_LOGIC_VECTOR (4 downto 0) := "00110";
    signal q0: std_logic;
    signal d : STD_LOGIC_VECTOR (0 downto 0);
    signal d0 : STD_LOGIC_VECTOR (0 downto 0);
begin

fop_fcmp_32ns_32ns_1_2_no_dsp_1_U1 : component fop_fcmp_32ns_32ns_1_2_no_dsp_1
    generic map (
        ID => 1,
        NUM_STAGE => 2,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
        dout_WIDTH => 1)
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        din0 => din0,
        din1 => din1,
        opcode => opcode,
        dout => d);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                d0 <= d;
            elsif (nready='1') then
                q0 <= pvalid;
                d0 <= d;
             end if;
          end if;
       end process;

       valid <= q0;
       dout <= d when nready='1' else d0;

end architecture;

-----------------------------------------------------------------------
-- fcmp oeq, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_oeq_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oeq_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (oeq)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;

-----------------------------------------------------------------------
-- fcmp ogt, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ogt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ogt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (ogt)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- fcmp oge, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_oge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oge_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (oge)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- fcmp olt, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_olt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_olt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (olt)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- fcmp ole, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_ole_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ole_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (ole)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- fcmp one, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_one_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_one_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_floatCmp (one)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;

-----------------------------------------------------------------------
-- dcmp alu, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;

entity ALU_doubleCmp is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : inout STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : inout STD_LOGIC_VECTOR(0 DOWNTO 0));
end entity;

architecture oeq of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00001",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture ogt of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00010",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture oge of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00011",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture olt of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00100",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture ole of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00101",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

architecture one of ALU_doubleCmp is

component dop_dcmp_64ns_64ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (63 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : inout STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0: std_logic;
begin

dop_dcmp_64ns_64ns_1_1_1_U1 : component dop_dcmp_64ns_64ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 64,
        din1_WIDTH => 64,
        dout_WIDTH => 1)
    port map (
        din0 => din0,
        din1 => din1,
        opcode => "00110",
        dout => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
            elsif (nready='1') then
                q0 <= pvalid;
             end if;
          end if;
       end process;

       valid <= q0;

end architecture;

-----------------------------------------------------------------------
-- dcmp oeq, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_oeq_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_oeq_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (oeq)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;

-----------------------------------------------------------------------
-- dcmp ogt, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_ogt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_ogt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (ogt)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- dcmp oge, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_oge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_oge_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (oge)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- dcmp olt, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_olt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_olt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (olt)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- dcmp ole, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_ole_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_ole_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (ole)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;
-----------------------------------------------------------------------
-- dcmp one, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dcmp_one_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_one_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.dass_join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      cmp_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    cmp: entity work.ALU_doubleCmp (one)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nReadyArray(0),         --nready,
                     cmp_valid, --valid,
                     cmp_ready, --ready,
                     dataInArray(DATA_SIZE_IN-1 downto 0),           --din0
                     dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0) <= '0';
                    end if;
                   end if;
             end if;
    end process; 

end architecture;

-----------------------------------------------------------------------
-- fcmp ord, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp uno, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp ueq, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp uge, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp ult, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp ule, version 0.0
-- TODO
-----------------------------------------------------------------------
-----------------------------------------------------------------------
-- fcmp une, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fcmp_une_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
    clk, rst : in std_logic; 
    dataInArray : in std_logic_vector(2*DATA_SIZE_IN-1 downto 0); 
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : inout std_logic_vector(0 downto 0);
    readyArray : inout std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_une_op is
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray <= zero when (dataInArray(DATA_SIZE_IN-1 downto 0) = dataInArray(2*DATA_SIZE_IN-1 downto DATA_SIZE_IN) ) else one;
    validArray(0) <= join_valid;

end architecture;
    

-----------------------------------------------------------------------
-- fcmp ugt, version 0.0
-- TODO
-----------------------------------------------------------------------
----------------------------------------------------------------------- 
-- float neg, version 0.0
-----------------------------------------------------------------------
----------------------------------------------------------------------- 
-- unsigned int division, version 0.0
-----------------------------------------------------------------------
----------------------------------------------------------------------- 
-- srem, remainder of signed division, version 0.0
-----------------------------------------------------------------------
----------------------------------------------------------------------- 
-- urem, remainder of unsigned division, version 0.0
-----------------------------------------------------------------------
----------------------------------------------------------------------- 
-- frem, remainder of float division, version 0.0
-----------------------------------------------------------------------
-------------------
--sinf
----------------
-------------------
--cosf
----------------
-------------------
--sqrtf
----------------
-------------------
--expf
----------------
-------------------
--exp2f
----------------
-------------------
--logf
----------------
-------------------
--log2f
----------------
-------------------
--log10f
----------------
-------------------
--fabsf
----------------
-------------------
--trunc_op
----------------
-------------------
--floorf_op
----------------
-------------------
--ceilf_op
----------------
-------------------
--roundf_op
----------------
-------------------
--fminf_op
----------------
-------------------
--fmaxf_op
----------------
-------------------
--powf_op
----------------
-------------------
--fabsf_op
----------------
-------------------
--copysignf_op
----------------
