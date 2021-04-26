-----------------------------------------------------------------------
-- int mul, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;

entity int_mul is

    port (
        clk : IN STD_LOGIC;
        reset : IN STD_LOGIC;
        pvalid : IN STD_LOGIC;
        nready : IN STD_LOGIC;
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of int_mul is

    component array_RAM_mul_32sbkb_MulnS_0 is
        port (
            clk : IN STD_LOGIC;
            ce : IN STD_LOGIC;
            a : IN STD_LOGIC_VECTOR;
            b : IN STD_LOGIC_VECTOR;
            p : OUT STD_LOGIC_VECTOR);
    end component;

    signal q0, q1, q2, q3, q4, q5, q6: std_logic;


begin
    multiply_unit :  component array_RAM_mul_32sbkb_MulnS_0
    port map (
        clk => clk,
        ce => nready,
        a => din0,
        b => din1,
        p => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
             end if;
          end if;
       end process;

       valid <= q3;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity mul_op is
Generic (
 INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : OUT std_logic_vector(1 downto 0);
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of mul_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);
signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      alu_ready,     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

     multiply: entity work.int_mul (arch)
            port map (clk, rst,
                     join_valid,     --pvalid,
                     nready_tmp,         --nready,
                     alu_valid, --valid,
                     alu_ready, --ready,
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

dataOutArray(0) <= alu_out;
validArray(0) <= alu_valid;
nready_tmp <= nReadyArray(0);

end architecture;

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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of int_div is

    component intop_sdiv_32ns_32ns_32_36_1_div is
    generic (
        in0_WIDTH   : INTEGER :=32;
        in1_WIDTH   : INTEGER :=32;
        out_WIDTH   : INTEGER :=32);
    port (
        clk         : in  STD_LOGIC;
        reset       : in  STD_LOGIC;
        ce          : in  STD_LOGIC;
        dividend    : in  STD_LOGIC_VECTOR(in0_WIDTH-1 downto 0);
        divisor     : in  STD_LOGIC_VECTOR(in1_WIDTH-1 downto 0);
        quot        : out STD_LOGIC_VECTOR(out_WIDTH-1 downto 0);
        remd        : out STD_LOGIC_VECTOR(out_WIDTH-1 downto 0));
    end component;

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20, q21, q22, q23, q24, q25, q26, q27, q28, q29, q30, q31, q32, q33, q34: std_logic;
    signal remd : STD_LOGIC_VECTOR(31 downto 0);

begin
    intop_sdiv_32ns_32ns_32_36_1_div_U1 :  component intop_sdiv_32ns_32ns_32_36_1_div
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        dividend => din0,
        divisor => din1,
        quot => dout,
        remd => remd);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
                q7 <= '0';
                q8 <= '0';
                q9 <= '0';
                q10 <= '0';
                q11 <= '0';
                q12 <= '0';
                q13 <= '0';
                q14 <= '0';
                q15 <= '0';
                q16 <= '0';
                q17 <= '0';
                q18 <= '0';
                q19 <= '0';
                q20 <= '0';
                q21 <= '0';
                q22 <= '0';
                q23 <= '0';
                q24 <= '0';
                q25 <= '0';
                q26 <= '0';
                q27 <= '0';
                q28 <= '0';
                q29 <= '0';
                q30 <= '0';
                q31 <= '0';
                q32 <= '0';
                q33 <= '0';
                q34 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
                q7 <= q6;
                q8 <= q7;
                q9 <= q8;
                q10 <= q9;
                q11 <= q10;
                q12 <= q11;
                q13 <= q12;
                q14 <= q13;
                q15 <= q14;
                q16 <= q15;
                q17 <= q16;
                q18 <= q17;
                q19 <= q18;
                q20 <= q19;
                q21 <= q20;
                q22 <= q21;
                q23 <= q22;
                q24 <= q23;
                q25 <= q24;
                q26 <= q25;
                q27 <= q26;
                q28 <= q27;
                q29 <= q28;
                q30 <= q29;
                q31 <= q30;
                q32 <= q31;
                q33 <= q32;
                q34 <= q33;
             end if;
          end if;
       end process;

       valid <= q34;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity div_op is
Generic (
 INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : OUT std_logic_vector(1 downto 0);
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of div_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);
signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of int_mod is

    component intop_sdiv_32ns_32ns_32_36_1_div is
    generic (
        in0_WIDTH   : INTEGER :=32;
        in1_WIDTH   : INTEGER :=32;
        out_WIDTH   : INTEGER :=32);
    port (
        clk         : in  STD_LOGIC;
        reset       : in  STD_LOGIC;
        ce          : in  STD_LOGIC;
        dividend    : in  STD_LOGIC_VECTOR(in0_WIDTH-1 downto 0);
        divisor     : in  STD_LOGIC_VECTOR(in1_WIDTH-1 downto 0);
        quot        : out STD_LOGIC_VECTOR(out_WIDTH-1 downto 0);
        remd        : out STD_LOGIC_VECTOR(out_WIDTH-1 downto 0));
    end component;

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20, q21, q22, q23, q24, q25, q26, q27, q28, q29, q30, q31, q32, q33, q34: std_logic;
    signal dummy : STD_LOGIC_VECTOR(31 downto 0);

begin
    intop_sdiv_32ns_32ns_32_36_1_div_U1 :  component intop_sdiv_32ns_32ns_32_36_1_div
    port map (
        clk => clk,
        reset => reset,
        ce => nready,
        dividend => din0,
        divisor => din1,
        quot => dummy,
        remd => dout);

    ready<= nready;

       process (clk) is
       begin
          if rising_edge(clk) then  
            if (reset = '1') then
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
                q7 <= '0';
                q8 <= '0';
                q9 <= '0';
                q10 <= '0';
                q11 <= '0';
                q12 <= '0';
                q13 <= '0';
                q14 <= '0';
                q15 <= '0';
                q16 <= '0';
                q17 <= '0';
                q18 <= '0';
                q19 <= '0';
                q20 <= '0';
                q21 <= '0';
                q22 <= '0';
                q23 <= '0';
                q24 <= '0';
                q25 <= '0';
                q26 <= '0';
                q27 <= '0';
                q28 <= '0';
                q29 <= '0';
                q30 <= '0';
                q31 <= '0';
                q32 <= '0';
                q33 <= '0';
                q34 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
                q7 <= q6;
                q8 <= q7;
                q9 <= q8;
                q10 <= q9;
                q11 <= q10;
                q12 <= q11;
                q13 <= q12;
                q14 <= q13;
                q15 <= q14;
                q16 <= q15;
                q17 <= q16;
                q18 <= q17;
                q19 <= q18;
                q20 <= q19;
                q21 <= q20;
                q22 <= q21;
                q23 <= q22;
                q24 <= q23;
                q25 <= q24;
                q26 <= q25;
                q27 <= q26;
                q28 <= q27;
                q29 <= q28;
                q30 <= q29;
                q31 <= q30;
                q32 <= q31;
                q33 <= q32;
                q34 <= q33;
             end if;
          end if;
       end process;

       valid <= q34;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity mod_op is
Generic (
 INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk : IN STD_LOGIC;
        rst : IN STD_LOGIC;
        pValidArray : IN std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : OUT std_logic_vector(1 downto 0);
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0));
end entity;

architecture arch of mod_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);
signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of ALU_doubleAdd is

    component dop_dadd_64ns_64nbkb IS
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
        dout : OUT STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;

begin

    dop_dadd_64ns_64nbkb_U1 : component dop_dadd_64ns_64nbkb
    generic map (
        ID => 1,
        NUM_STAGE => 5,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                --q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                --q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q3;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dadd_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dadd_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of ALU_doubleSub is

    component dop_dsub_64ns_64ncud IS
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
        dout : OUT STD_LOGIC_VECTOR (63 downto 0) );
    end component;

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;

begin

    dop_dsub_64ns_64ncud_U1 : component dop_dsub_64ns_64ncud
    generic map (
        ID => 2,
        NUM_STAGE => 5,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                --q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                --q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q3;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dsub_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dsub_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of doubleMul is

component dop_dmul_64ns_64ndEe IS
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
        dout : OUT STD_LOGIC_VECTOR (63 downto 0) );
    end component;
    signal q0, q1, q2, q3, q4, q5, q6: std_logic;


begin

 dop_dmul_64ns_64ndEe_U1 : component dop_dmul_64ns_64ndEe
    generic map (
        ID => 3,
        NUM_STAGE => 6,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q4;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity dmul_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dmul_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(63 DOWNTO 0));
end entity;

architecture arch of doubleDiv is

component dop_ddiv_64ns_64neOg IS
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
        dout : OUT STD_LOGIC_VECTOR (63 downto 0) );
    end component;
    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20, q21, q22, q23, q24, q25, q26, q27, q28, q29: std_logic;


begin

 dop_ddiv_64ns_64neOg_U1 : component dop_ddiv_64ns_64neOg
    generic map (
        ID => 4,
        NUM_STAGE => 31,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
                q7 <= '0';
                q8 <= '0';
                q9 <= '0';
                q10 <= '0';
                q11 <= '0';
                q12 <= '0';
                q13 <= '0';
                q14 <= '0';
                q15 <= '0';
                q16 <= '0';
                q17 <= '0';
                q18 <= '0';
                q19 <= '0';
                q20 <= '0';
                q21 <= '0';
                q22 <= '0';
                q23 <= '0';
                q24 <= '0';
                q25 <= '0';
                q26 <= '0';
                q27 <= '0';
                q28 <= '0';
                q29 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
                q7 <= q6;
                q8 <= q7;
                q9 <= q8;
                q10 <= q9;
                q11 <= q10;
                q12 <= q11;
                q13 <= q12;
                q14 <= q13;
                q15 <= q14;
                q16 <= q15;
                q17 <= q16;
                q18 <= q17;
                q19 <= q18;
                q20 <= q19;
                q21 <= q20;
                q22 <= q21;
                q23 <= q22;
                q24 <= q23;
                q25 <= q24;
                q26 <= q25;
                q27 <= q26;
                q28 <= q27;
                q29 <= q28;
             end if;
          end if;
       end process;

       valid <= q29;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ddiv_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of ddiv_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatAdd is

    component fop_fadd_32ns_32nbkb IS
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

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;

begin

    fop_fadd_32ns_32nbkb_U1 : component fop_fadd_32ns_32nbkb
    generic map (
        ID => 1,
        NUM_STAGE => 5,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                --q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                --q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q3;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fadd_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fadd_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
    validArray(0) <= alu_valid;

end architecture;

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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of ALU_floatSub is

    component fop_fsub_32ns_32ncud IS
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

    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;

begin

    fop_fsub_32ns_32ncud_U1 : component fop_fsub_32ns_32ncud
    generic map (
        ID => 2,
        NUM_STAGE => 5,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                --q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                --q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q3;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fsub_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fsub_op is

signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of floatMul is

component fop_fmul_32ns_32ndEe IS
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
    signal q0, q1, q2, q3, q4, q5, q6: std_logic;


begin

 fop_fmul_32ns_32ndEe_U1 : component fop_fmul_32ns_32ndEe
    generic map (
        ID => 3,
        NUM_STAGE => 4,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                --q3 <= '0';
                --q4 <= '0';
               -- q5 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                --q3 <= q2;
                --q4 <= q3;
                --q5 <= q4;
                --q6 <= q5;
             end if;
          end if;
       end process;

       valid <= q2;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fmul_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fmul_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(31 DOWNTO 0));
end entity;

architecture arch of floatDiv is

component fop_fdiv_32ns_32neOg IS
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
    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11, q12, q13, q14: std_logic;


begin

 fop_fdiv_32ns_32neOg_U1 : component fop_fdiv_32ns_32neOg
    generic map (
        ID => 4,
        NUM_STAGE => 16,
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
                q0 <= pvalid;
                q1 <= '0';
                q2 <= '0';
                q3 <= '0';
                q4 <= '0';
                q5 <= '0';
                q6 <= '0';
                q7 <= '0';
                q8 <= '0';
                q9 <= '0';
                q10 <= '0';
                q11 <= '0';
                q12 <= '0';
                q13 <= '0';
                q14 <= '0';
            elsif (nready='1') then
                q0 <= pvalid;
                q1 <= q0;
                q2 <= q1;
                q3 <= q2;
                q4 <= q3;
                q5 <= q4;
                q6 <= q5;
                q7 <= q6;
                q8 <= q7;
                q9 <= q8;
                q10 <= q9;
                q11 <= q10;
                q12 <= q11;
                q13 <= q12;
                q14 <= q13;
             end if;
          end if;
       end process;

       valid <= q14;

end architecture;

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity fdiv_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
    port (
        clk, rst : in std_logic; 
        dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
        dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
        pValidArray : in std_logic_vector(1 downto 0);
        nReadyArray : in std_logic_vector(0 downto 0);
        validArray : out std_logic_vector(0 downto 0);
        readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fdiv_op is

   signal alu_out: STD_LOGIC_VECTOR (DATA_SIZE_OUT-1 downto 0);
signal alu_valid, alu_ready : STD_LOGIC;

signal join_valid : STD_LOGIC;
signal join_ReadyArray : std_logic_vector(1 downto 0);

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     alu_out);  --dout

    dataOutArray(0) <= alu_out;
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(0 DOWNTO 0));
end entity;

architecture oeq of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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

architecture ogt of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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

architecture oge of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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

architecture olt of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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

architecture ole of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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

architecture one of ALU_floatCmp is

component fop_fcmp_32ns_32ns_1_1_1 IS
    generic (
        ID : INTEGER;
        NUM_STAGE : INTEGER;
        din0_WIDTH : INTEGER;
        din1_WIDTH : INTEGER;
        dout_WIDTH : INTEGER );
    port (
        din0 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR (31 DOWNTO 0);
        opcode : IN STD_LOGIC_VECTOR (4 downto 0);
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
begin

fop_fcmp_32ns_32ns_1_1_1_U1 : component fop_fcmp_32ns_32ns_1_1_1
    generic map (
        ID => 5,
        NUM_STAGE => 1,
        din0_WIDTH => 32,
        din1_WIDTH => 32,
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oeq_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ogt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_oge_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_olt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_ole_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of fcmp_one_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
        valid, ready : OUT STD_LOGIC;
        din0 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        din1 : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
        dout : OUT STD_LOGIC_VECTOR(0 DOWNTO 0));
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
        dout : OUT STD_LOGIC_VECTOR (0 downto 0) );
    end component;


    signal q0, q1, q2, q3, q4, q5, q6, q7, q8, q9: std_logic;
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_oeq_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_ogt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_oge_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_olt_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_ole_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of dcmp_one_op is

    signal cmp_out: STD_LOGIC_VECTOR (0 downto 0);
    signal cmp_valid, cmp_ready : STD_LOGIC;

    signal join_valid : STD_LOGIC;
    signal join_ReadyArray : std_logic_vector(1 downto 0);
    signal nready_tmp : std_logic;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
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
                     dataInArray(0),           --din0
                     dataInArray(1),           --din1
                     cmp_out);  --dout

    process(clk, rst) is

          begin

           if (rst = '1') then

                   validArray(0)  <= '0';
                   dataOutArray(0)(0) <= '0';
                               
            elsif (rising_edge(clk)) then
                  if (cmp_valid= '1') then
                        validArray(0)   <= '1';
                        dataOutArray(0) <= cmp_out;
                    else
                      if (nReadyArray(0) = '1') then
                         validArray(0)  <= '0';
                         dataOutArray(0)(0) <= '0';
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
-- TODO
-----------------------------------------------------------------------
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