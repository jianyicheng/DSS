----------------------------------------------------------------------- 
-- ret, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ret_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (0 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(0 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(0 downto 0));
end entity;

architecture arch of ret_op is

begin 

    dataOutArray(0) <= dataInArray(0);
    validArray(0) <= pValidArray(0);
    readyArray(0) <= nReadyArray(0);

end architecture;

----------------------------------------------------------------------- 
-- int add, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity add_op is
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

architecture arch of add_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) + unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- int sub, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity sub_op is
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

architecture arch of sub_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(unsigned(dataInArray(0)) - unsigned (dataInArray(1)));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- logic and, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity and_op is
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

architecture arch of and_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) and dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- logic or, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity or_op is
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

architecture arch of or_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) or dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- logic xor, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity xor_op is
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

architecture arch of xor_op is

signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= dataInArray(0) xor dataInArray(1);
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- sext, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity sext_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (0 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(0 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(0 downto 0));
end entity;

architecture arch of sext_op is

    signal join_valid : STD_LOGIC;

begin 

    dataOutArray(0)<= std_logic_vector(IEEE.numeric_std.resize(signed(dataInArray(0)),DATA_SIZE_OUT));
    validArray <= pValidArray;
    readyArray(0) <= not pValidArray(0) or (pValidArray(0) and nReadyArray(0));

end architecture;

-----------------------------------------------------------------------
-- zext, version 0.0
-- TODO check signed/unsigned
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity zext_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (0 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(0 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(0 downto 0));
end entity;

architecture arch of zext_op is

    signal join_valid : STD_LOGIC;

begin 

    dataOutArray(0)<= std_logic_vector(IEEE.numeric_std.resize(signed(dataInArray(0)),DATA_SIZE_OUT));
    validArray <= pValidArray;
    readyArray(0) <= not pValidArray(0) or (pValidArray(0) and nReadyArray(0));

end architecture;
-----------------------------------------------------------------------
-- shl, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity shl_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of shl_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_left(unsigned(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- ashr, version 0.0
-----------------------------------------------------------------------
Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity ashr_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of ashr_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_right(signed(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- lshr, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity lshr_op is
Generic (
  INPUTS: integer; OUTPUTS: integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
  clk, rst : in std_logic; 
  dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
  dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
  pValidArray : in std_logic_vector(1 downto 0);
  nReadyArray : in std_logic_vector(0 downto 0);
  validArray : out std_logic_vector(0 downto 0);
  readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of lshr_op is

    signal join_valid : STD_LOGIC;

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= std_logic_vector(shift_right(unsigned(dataInArray(0)),to_integer(unsigned('0' & dataInArray(1)(DATA_SIZE_IN-2 downto 0)))));
    validArray(0) <= join_valid;


end architecture;

-----------------------------------------------------------------------
-- select, version 0.0
-----------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity antitokens is port(
  clk, reset : in std_logic;
  pvalid1, pvalid0 : in std_logic;
  kill1, kill0 : out std_logic;
  generate_at1, generate_at0 : in std_logic;
  stop_valid : out std_logic);

end antitokens;


architecture arch of antitokens is

signal reg_in0, reg_in1, reg_out0, reg_out1 : std_logic;
  
begin

  reg0 : process(clk, reset, reg_in0)
    begin
      if(reset='1') then
        reg_out0 <= '0'; 
      else
        if(rising_edge(clk))then
          reg_out0 <= reg_in0;
        end if;
      end if;
    end process reg0;

  reg1 : process(clk, reset, reg_in1)
    begin
      if(reset='1') then
        reg_out1 <= '0'; 
      else
        if(rising_edge(clk))then
          reg_out1 <= reg_in1;
        end if;
      end if;
    end process reg1;

  reg_in0 <= not pvalid0 and (generate_at0 or reg_out0);
  reg_in1 <= not pvalid1 and (generate_at1 or reg_out1);

  stop_valid <= reg_out0 or reg_out1;

  kill0 <= generate_at0 or reg_out0;
  kill1 <= generate_at1 or reg_out1;

end arch;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
USE work.customTypes.all;

entity select_op is 
Generic (
 INPUTS : integer ; OUTPUTS : integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
-- llvm select: operand(0) is condition, operand(1) is true, operand(2) is false
-- here, dataInArray(0) is true, dataInArray(1) is false operand
port (
    clk, rst :      in std_logic;
    dataInArray :   in data_array (1 downto 0)(DATA_SIZE_IN - 1 downto 0);
    dataOutArray :  out data_array (0 downto 0)(DATA_SIZE_OUT - 1 downto 0);    
    pValidArray :   in std_logic_vector(2 downto 0);
    nReadyArray :   in std_logic_vector(0 downto 0);
    validArray :    out std_logic_vector(0 downto 0);
    readyArray :    out std_logic_vector(2 downto 0);
    condition: in data_array (0 downto 0)(0 downto 0)
  );

  end select_op;

architecture arch of select_op is
    signal  ee, validInternal : std_logic;
    signal  kill0, kill1 : std_logic;
  signal  antitokenStop:  std_logic;
  signal  g0, g1: std_logic;


    begin
      
      ee <= pValidArray(0) and ((not condition(0)(0) and pValidArray(2)) or (condition(0)(0) and pValidArray(1))); --condition and one input
      validInternal <= ee and not antitokenStop; -- propagate ee if not stopped by antitoken

      g0 <= not pValidArray(1) and validInternal and nReadyArray(0);
      g1 <= not pValidArray(2) and validInternal and nReadyArray(0);

      validArray(0) <= validInternal;
      readyArray(1) <= (not pValidArray(1)) or (validInternal and nReadyArray(0)) or kill0; -- normal join or antitoken
      readyArray(2) <= (not pValidArray(2)) or (validInternal and nReadyArray(0)) or kill1; --normal join or antitoken
      readyArray(0) <= (not pValidArray(0)) or (validInternal and nReadyArray(0)); --like normal join

      dataOutArray(0) <= dataInArray(1) when (condition(0)(0) = '0') else dataInArray(0);

      Antitokens: entity work.antitokens
        port map ( clk, rst, 
        pValidArray(2), pValidArray(1), 
              kill1, kill0,
              g1, g0, 
              antitokenStop);
    end arch;


-----------------------------------------------------------------------
-- icmp eq, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_eq_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_eq_op is
--eq: yields true if the operands are equal, false otherwise. No sign interpretation is necessary or performed.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (dataInArray(0) = dataInArray(1) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ne, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ne_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ne_op is
--ne: yields true if the operands are unequal, false otherwise. No sign interpretation is necessary or performed.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= zero when (dataInArray(0) = dataInArray(1) ) else one;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ugt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ugt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ugt_op is
--ugt: interprets the operands as unsigned values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) > unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp uge, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_uge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_uge_op is
--uge: interprets the operands as unsigned values and yields true if op1 is greater than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) >= unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sgt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sgt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sgt_op is
-- sgt: interprets the operands as signed values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";
    signal res: std_logic_vector (0 downto 0);
    signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) > signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sge, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sge_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sge_op is
-- sge: interprets the operands as signed values and yields true if op1 is greater than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";
    signal res: std_logic_vector (0 downto 0);
    signal nready_tmp : std_logic;
begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) >= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ult, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ult_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ult_op is
--ugt: interprets the operands as unsigned values and yields true if op1 is greater than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) < unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp ule, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_ule_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_ule_op is
-- ule: interprets the operands as unsigned values and yields true if op1 is less than or equal to op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (unsigned(dataInArray(0)) <= unsigned(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp slt, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_slt_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_slt_op is
-- slt: interprets the operands as signed values and yields true if op1 is less than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) < signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

-----------------------------------------------------------------------
-- icmp sle, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;

entity icmp_sle_op is
Generic (
INPUTS:integer; OUTPUTS:integer; DATA_SIZE_IN: integer; DATA_SIZE_OUT: integer
);
port(
    clk, rst : in std_logic; 
    dataInArray : in data_array (1 downto 0)(DATA_SIZE_IN-1 downto 0); 
    dataOutArray : out data_array (0 downto 0)(DATA_SIZE_OUT-1 downto 0);      
    pValidArray : in std_logic_vector(1 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    validArray : out std_logic_vector(0 downto 0);
    readyArray : out std_logic_vector(1 downto 0));
end entity;

architecture arch of icmp_sle_op is
-- slt: interprets the operands as signed values and yields true if op1 is less than op2.
    signal join_valid : STD_LOGIC;
    signal one: std_logic_vector (0 downto 0) := "1";
    signal zero: std_logic_vector (0 downto 0) := "0";

begin 

    join_write_temp:   entity work.join(arch) generic map(2)
            port map( pValidArray,  --pValidArray
                      nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                      readyArray);   --readyarray 

    dataOutArray(0) <= one when (signed(dataInArray(0)) <= signed(dataInArray(1)) ) else zero;
    validArray(0) <= join_valid;

end architecture;

----------------------------------------------------------------------- 
-- getelementptr, version 0.0
-----------------------------------------------------------------------

Library IEEE;
use IEEE.std_logic_1164.all;
use ieee.numeric_std.all;
use work.customTypes.all;
entity getelementptr_op is
Generic ( INPUTS: integer; OUTPUTS : Integer; INPUT_SIZE: Integer; OUTPUT_SIZE : Integer; CONST_SIZE : Integer);

    -- component inputs: i, j, k,... dimx, dimy, dimz
    -- inputs: total number of inputs
    -- outputs: total number of outputs
    -- input/output size: bitwidths
    -- const_size: number of dimensions (dimx, ..)

port(
  clk : IN STD_LOGIC;
  rst : IN STD_LOGIC;
  pValidArray : IN std_logic_vector(INPUTS - 1 downto 0);
  nReadyArray : IN STD_LOGIC_VECTOR(0 downto 0);
  validArray : OUT STD_LOGIC_VECTOR(0 downto 0);
  readyArray : OUT std_logic_vector(INPUTS -1 downto 0);
  dataInArray: IN data_array(INPUTS-1 downto 0)(INPUT_SIZE - 1 DOWNTO 0);
  dataOutArray : OUT data_array(0 downto 0)(OUTPUT_SIZE - 1 DOWNTO 0));
end entity;


architecture arch of  getelementptr_op is

    signal join_valid : STD_LOGIC;


begin 

    -- join only for variable inputs
    join_write_temp:   entity work.join(arch) generic map(INPUTS - CONST_SIZE)
            port map( pValidArray(INPUTS - CONST_SIZE - 1 downto 0),  --pValidArray
                nReadyArray(0),     --nready                    
                      join_valid,         --valid          
                readyArray(INPUTS - CONST_SIZE - 1 downto 0));   --readyarray 

    readyArray (INPUTS -1 downto INPUTS - CONST_SIZE) <=  (others => '1');

    validArray(0) <= join_valid;

    -- convert index [i][j][k] or array[dimX][dimY][dimZ] into index [i * dimY*dimZ + j * dimZ + k] 
    process(dataInArray)
        variable tmp_data_out  : unsigned(INPUT_SIZE - 1 downto 0);
        variable tmp_const  : integer;
        variable tmp_mul  : integer;

    begin
        tmp_data_out  := (others => '0');

        for I in 0 to INPUTS - CONST_SIZE - 1 loop
          tmp_const  := 1;
          for J in INPUTS - CONST_SIZE + I to INPUTS - 1 loop
              tmp_const  := tmp_const * to_integer(unsigned(dataInArray(J)));
          end loop;
          tmp_mul := to_integer(unsigned(dataInArray(I))) * tmp_const; 
          tmp_data_out  := tmp_data_out + to_unsigned(tmp_mul, 32);
        end loop;
    dataOutArray(0)  <= std_logic_vector(resize(tmp_data_out, OUTPUT_SIZE));

    end process;

end architecture;
