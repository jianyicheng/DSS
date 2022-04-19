library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity read_priority is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        req          : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); -- read requests (pValid signals)
        data_ready   : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); -- ready from next 
        priority_out : inout std_logic_vector(ARBITER_SIZE - 1 downto 0) -- priority function output
    );
end entity;

architecture arch of read_priority is

begin
    process(req, data_ready)
        variable prio_req : std_logic;
    begin
        -- the first index I such that (req(I) and data_ready(I) = '1') is '1', others are '0'
        priority_out(0) <= req(0) and data_ready(0);
        prio_req        := req(0) and data_ready(0);
        for I in 1 to ARBITER_SIZE - 1 loop
            priority_out(I) <= (not prio_req) and req(I) and data_ready(I);
            prio_req        := prio_req or (req(I) and data_ready(I));
        end loop;
    end process;
end architecture;

--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity read_address_mux is
    generic(
        ARBITER_SIZE : natural;
        ADDR_WIDTH   : natural
    );
    port(
        sel      : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        addr_in  : in  std_logic_vector(ARBITER_SIZE*ADDR_WIDTH - 1 downto 0);
        addr_out : out std_logic_vector(ADDR_WIDTH - 1 downto 0)
    );
end entity;

architecture arch of read_address_mux is

begin
    process(sel, addr_in)
        variable addr_out_var : std_logic_vector(ADDR_WIDTH - 1 downto 0);
    begin
        addr_out_var := (others => '0');
        for I in 0 to ARBITER_SIZE - 1 loop
            if (sel(I) = '1') then
                addr_out_var := addr_in(I*ADDR_WIDTH+ADDR_WIDTH-1 downto I*ADDR_WIDTH);
            end if;
        end loop;
        addr_out     <= addr_out_var;
    end process;
end architecture;

--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity read_address_ready is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        sel    : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        nReady : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        ready  : inout std_logic_vector(ARBITER_SIZE - 1 downto 0)
    );
end entity;

architecture arch of read_address_ready is
begin
    GEN1 : for I in 0 to ARBITER_SIZE - 1 generate
        ready(I) <= nReady(I) and sel(I);
    end generate GEN1;
end architecture;

--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity read_data_signals is
    generic(
        ARBITER_SIZE : natural;
        DATA_WIDTH   : natural
    );
    port(
        ce: in std_logic;
        rst       : in  std_logic;
        clk       : in  std_logic;
        sel       : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        read_data : in  std_logic_vector(DATA_WIDTH - 1 downto 0);
        out_data  : inout std_logic_vector(ARBITER_SIZE*DATA_WIDTH - 1 downto 0);
        valid     : inout std_logic_vector(ARBITER_SIZE - 1 downto 0);
        nReady    : in  std_logic_vector(ARBITER_SIZE - 1 downto 0)
    );
end entity;

architecture arch of read_data_signals is
    signal sel_prev : std_logic_vector(ARBITER_SIZE - 1 downto 0);
    signal out_reg : std_logic_vector(ARBITER_SIZE*DATA_WIDTH - 1 downto 0);
begin

    process(clk, rst) is
    begin
        if (rst = '1') then
            for I in 0 to ARBITER_SIZE - 1 loop
                valid(I)    <= '0';
                sel_prev(I) <= '0';
            end loop;
        elsif (rising_edge(clk)) then
            if ce = '1' then
                for I in 0 to ARBITER_SIZE - 1 loop
                    sel_prev(I) <= sel(I);
                    if (sel(I) = '1') then 
                        valid(I)    <= '1';  --or not nReady(I); -- just sel(I) ??
                        --sel_prev(I) <= '1';
                    else 
                        if (nReady(I) = '1') then
                            valid(I)  <= '0';
                            ---sel_prev(I) <= '0';
                        end if;
                    end if;
                end loop;
            end if;
        end if;
    end process;

    process(clk, rst) is
    begin
        if (rising_edge(clk)) then
            if ce = '1' then
                for I in 0 to ARBITER_SIZE - 1 loop
                        if (sel_prev(I) = '1') then
                            out_reg(I*DATA_WIDTH+DATA_WIDTH-1 downto I*DATA_WIDTH) <= read_data;
                        end if;
                end loop;
            end if;
         end if;
    end process;

    process(read_data, sel_prev, out_reg) is
    begin
        for I in 0 to ARBITER_SIZE - 1 loop
            if (sel_prev(I) = '1') then
                out_data(I*DATA_WIDTH+DATA_WIDTH-1 downto I*DATA_WIDTH) <= read_data;
            else
                out_data(I*DATA_WIDTH+DATA_WIDTH-1 downto I*DATA_WIDTH) <= out_reg(I*DATA_WIDTH+DATA_WIDTH-1 downto I*DATA_WIDTH);
            end if;
        end loop;
    end process;

end architecture;



--------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity read_memory_arbiter is
    generic(
        ARBITER_SIZE : natural := 2;
        ADDR_WIDTH   : natural := 32;
        DATA_WIDTH   : natural := 32
    );
    port(
        rst              : in  std_logic;
        clk              : in  std_logic;
        ce              : in  std_logic;
        --- interface to previous
        pValid           : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); -- read requests
        ready            : inout std_logic_vector(ARBITER_SIZE - 1 downto 0); -- ready to process read
        address_in       : in  std_logic_vector(ARBITER_SIZE*ADDR_WIDTH - 1 downto 0);
        ---interface to next
        nReady           : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); -- next component can accept data
        valid            : inout std_logic_vector(ARBITER_SIZE - 1 downto 0); -- sending data to next component
        data_out         : inout std_logic_vector(ARBITER_SIZE*DATA_WIDTH - 1 downto 0); -- data to next components

        ---interface to memory
        read_enable      : out std_logic;
        read_address     : out std_logic_vector(ADDR_WIDTH - 1 downto 0);
        data_from_memory : in  std_logic_vector(DATA_WIDTH - 1 downto 0));

end entity;

architecture arch of read_memory_arbiter is
    signal priorityOut : std_logic_vector(ARBITER_SIZE - 1 downto 0);

begin

    priority : entity work.read_priority
        generic map(
            ARBITER_SIZE => ARBITER_SIZE
        )
        port map(
            req          => pValid,
            data_ready   => nReady,
            priority_out => priorityOut
        );

    addressing : entity work.read_address_mux
        generic map(
            ARBITER_SIZE => ARBITER_SIZE,
            ADDR_WIDTH   => ADDR_WIDTH
        )
        port map(
            sel      => priorityOut,
            addr_in  => address_in,
            addr_out => read_address
        );

    adderssReady : entity work.read_address_ready
        generic map(
            ARBITER_SIZE => ARBITER_SIZE
        )
        port map(
            sel    => priorityOut,
            nReady => nReady,
            ready  => ready
        );

    data : entity work.read_data_signals
        generic map(
            ARBITER_SIZE => ARBITER_SIZE,
            DATA_WIDTH   => DATA_WIDTH
        )
        port map(
            ce        => ce,
            rst       => rst,
            clk       => clk,
            sel       => priorityOut,
            read_data => data_from_memory,
            out_data  => data_out,
            valid     => valid,
            nReady    => nReady
        );

    process(priorityOut) is
        variable read_en_var : std_logic;
    begin
        read_en_var := '0';
        for I in 0 to ARBITER_SIZE - 1 loop
            read_en_var := read_en_var or priorityOut(I);
        end loop;
        read_enable <= read_en_var;
    end process;

end architecture;


--------------------------------------------------

--------------------------------------------------
---write_priority----

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity write_priority is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        req          : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        data_ready   : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        priority_out : inout std_logic_vector(ARBITER_SIZE - 1 downto 0)
    );
end entity;

architecture arch of write_priority is

begin

    process(data_ready, req)
        variable prio_req : std_logic;

    begin
        -- the first index I such that (req(I) and data_ready(I) = '1') is '1', others are '0'
        priority_out(0) <= req(0) and data_ready(0);
        prio_req        := req(0) and data_ready(0);

        for I in 1 to ARBITER_SIZE - 1 loop
            priority_out(I) <= (not prio_req) and req(I) and data_ready(I);
            prio_req        := prio_req or (req(I) and data_ready(I));
        end loop;
    end process;
end architecture;

--------------------------------------------------
---write_address_mux----

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity write_address_mux is
    generic(
        ARBITER_SIZE : natural;
        ADDR_WIDTH   : natural
    );
    port(
        sel      : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        addr_in  : in  std_logic_vector(ARBITER_SIZE*ADDR_WIDTH - 1 downto 0);
        addr_out : out std_logic_vector(ADDR_WIDTH - 1 downto 0)
    );
end entity;

architecture arch of write_address_mux is

begin
    process(sel, addr_in)
        variable addr_out_var : std_logic_vector(ADDR_WIDTH - 1 downto 0);
    begin
        addr_out_var := (others => '0');
        for I in 0 to ARBITER_SIZE - 1 loop
            if (sel(I) = '1') then
                addr_out_var := addr_in(I*ADDR_WIDTH+ADDR_WIDTH-1 downto I*ADDR_WIDTH);
            end if;
        end loop;
        addr_out     <= addr_out_var;
    end process;
end architecture;

--------------------------------------------------
---write_address_ready----

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity write_address_ready is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        sel    : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        nReady : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        ready  : inout std_logic_vector(ARBITER_SIZE - 1 downto 0)
    );

end entity;

architecture arch of write_address_ready is

begin

    GEN1 : for I in 0 to ARBITER_SIZE - 1 generate
        ready(I) <= nReady(I) and sel(I);
    end generate GEN1;

end architecture;

--------------------------------------------------
---write_data_signals----

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity write_data_signals is
    generic(
        ARBITER_SIZE : natural;
        DATA_WIDTH   : natural
    );
    port(
        ce         : in std_logic;
        rst        : in  std_logic;
        clk        : in  std_logic;
        sel        : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        write_data : out std_logic_vector(DATA_WIDTH - 1 downto 0);
        in_data    : in  std_logic_vector(ARBITER_SIZE*DATA_WIDTH - 1 downto 0);
        valid      : inout std_logic_vector(ARBITER_SIZE - 1 downto 0)
    );

end entity;

architecture arch of write_data_signals is

begin

    process(sel, in_data)
        variable data_out_var : std_logic_vector(DATA_WIDTH - 1 downto 0);
    begin
        data_out_var := (others => '0');

        for I in 0 to ARBITER_SIZE - 1 loop
            if (sel(I) = '1') then
                data_out_var := in_data(I*DATA_WIDTH+DATA_WIDTH-1 downto I*DATA_WIDTH);
            end if;
        end loop;
        write_data <= data_out_var;
    end process;

    process(clk, rst) is
    begin
        if (rst = '1') then
            for I in 0 to ARBITER_SIZE - 1 loop
                valid(I) <= '0';
            end loop;

        elsif (rising_edge(clk)) then
            if ce = '1' then
                for I in 0 to ARBITER_SIZE - 1 loop
                    valid(I) <= sel(I);
                end loop;
            end if;
        end if;
    end process;
end architecture;

--------------------------------------------------
---write_memory_arbiter----

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity write_memory_arbiter is
    generic(
        ARBITER_SIZE : natural := 2;
        ADDR_WIDTH   : natural := 32;
        DATA_WIDTH   : natural := 32
    );
    port(
        ce: in std_logic;
        rst            : in  std_logic;
        clk            : in  std_logic;
        --- interface to previous
        pValid         : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); --write requests
        pValidData         : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); --write requests
        ready          : inout std_logic_vector(ARBITER_SIZE - 1 downto 0); -- ready
        readyData          : inout std_logic_vector(ARBITER_SIZE - 1 downto 0); -- ready
        address_in     : in  std_logic_vector(ARBITER_SIZE*ADDR_WIDTH - 1 downto 0);
        data_in        : in  std_logic_vector(ARBITER_SIZE*DATA_WIDTH - 1 downto 0); -- data from previous that want to write

        ---interface to next
        nReady         : in  std_logic_vector(ARBITER_SIZE - 1 downto 0); -- next component can continue after write
        valid          : inout std_logic_vector(ARBITER_SIZE - 1 downto 0); --sending write confirmation to next component

        ---interface to memory
        enable         : inout std_logic;
        write_address  : out std_logic_vector(ADDR_WIDTH - 1 downto 0);
        data_to_memory : out std_logic_vector(DATA_WIDTH - 1 downto 0)
    );

end entity;

architecture arch of write_memory_arbiter is
    signal priorityOut : std_logic_vector(ARBITER_SIZE - 1 downto 0);
    signal joinValid : std_logic_vector(ARBITER_SIZE - 1 downto 0);
begin

    process(pValid, priorityOut)
    begin
        for I in 0 to ARBITER_SIZE - 1 loop
            if pValid(I) = '0' then
                ready(I) <= '1';
            else
                ready(I) <= priorityOut(I);
            end if;
        end loop;
    end process;

    process(pValidData, priorityOut)
    begin
        for I in 0 to ARBITER_SIZE - 1 loop
            if pValidData(I) = '0' then
                readyData(I) <= '1';
            else
                readyData(I) <= priorityOut(I);
            end if;
        end loop;
    end process;

    joinValid <= pValid and pValidData;

    priority : entity work.write_priority
        generic map(
            ARBITER_SIZE => ARBITER_SIZE
        )
        port map(
            req          => joinValid, -- pValid,
            data_ready   => nReady,
            priority_out => priorityOut
        );

    addressing : entity work.write_address_mux
        generic map(
            ARBITER_SIZE => ARBITER_SIZE,
            ADDR_WIDTH   => ADDR_WIDTH
        )
        port map(
            sel      => priorityOut,
            addr_in  => address_in,
            addr_out => write_address
        );

    -- addressReady : entity work.write_address_ready
    --     generic map(
    --         ARBITER_SIZE => ARBITER_SIZE
    --     )
    --     port map(
    --         sel    => priorityOut,
    --         nReady => nReady,
    --         ready  => ready
    --     );

    data : entity work.write_data_signals
        generic map(
            ARBITER_SIZE => ARBITER_SIZE,
            DATA_WIDTH   => DATA_WIDTH
        )
        port map(
            ce         => ce,
            rst        => rst,
            clk        => clk,
            sel        => priorityOut,
            write_data => data_to_memory,
            in_data    => data_in,
            valid      => valid
        );

    process(priorityOut) is
        variable write_en_var : std_logic;
    begin
        write_en_var := '0';
        for I in 0 to ARBITER_SIZE - 1 loop
            write_en_var := write_en_var or priorityOut(I);
        end loop;
        enable       <= write_en_var;
    end process;
end architecture;


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
USE work.customTypes.all;

entity elasticBufferDummy is
Generic (
  SIZE :integer; INPUTS :integer:=32; DATA_SIZE_IN: integer;DATA_SIZE_OUT: integer 
);
port(
    clk, rst : in std_logic;  
    dataInArray : in std_logic_vector(DATA_SIZE_IN-1 downto 0);
    dataOutArray : inout std_logic_vector(DATA_SIZE_OUT-1 downto 0);
    ReadyArray : inout std_logic_vector(0 downto 0);
    ValidArray : inout std_logic_vector(0 downto 0);
    nReadyArray : in std_logic_vector(0 downto 0);
    pValidArray : in std_logic_vector(0 downto 0));
end elasticBufferDummy;
------------------------------------------------------------------------ 
-- elastic buffer 
------------------------------------------------------------------------ 
architecture arch of elasticBufferDummy is
    
begin

dataOutArray <= dataInArray;
ValidArray(0) <= pValidArray(0);
ReadyArray(0) <= nReadyArray(0);
    
end arch;

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;


entity mc_load_op is generic( INPUTS : Integer;  OUTPUTS : Integer; ADDRESS_SIZE : Integer;  DATA_SIZE : Integer);
port (
    rst: in std_logic;
    clk: in std_logic;

    --- interface to previous
    pValidArray : in std_logic_vector(INPUTS - 1 downto 0);
    readyArray : inout std_logic_vector(INPUTS - 1 downto 0);
    dataInArray: in std_logic_vector(DATA_SIZE -1 downto 0);
    input_addr: in std_logic_vector(ADDRESS_SIZE -1 downto 0);

    ---interface to next
    nReadyArray : in std_logic_vector(OUTPUTS - 1 downto 0); 
    validArray : inout std_logic_vector(OUTPUTS - 1 downto 0);
    dataOutArray : inout std_logic_vector(DATA_SIZE-1 downto 0);
    output_addr: inout std_logic_vector(ADDRESS_SIZE -1 downto 0)
    );

end entity;

architecture arch of mc_load_op is 
    signal Buffer_1_readyArray_0 : std_logic;
    signal Buffer_1_validArray_0 : std_logic;
    signal Buffer_1_dataOutArray_0 : std_logic_vector(ADDRESS_SIZE-1 downto 0);

    signal Buffer_2_readyArray_0 : std_logic;
    signal Buffer_2_validArray_0 : std_logic;
    signal Buffer_2_dataOutArray_0 : std_logic_vector(DATA_SIZE-1 downto 0);

    signal addr_from_circuit: std_logic_vector(ADDRESS_SIZE-1 downto 0);
    signal addr_from_circuit_valid: std_logic;
    signal addr_from_circuit_ready: std_logic;

    signal addr_to_lsq: std_logic_vector(ADDRESS_SIZE-1 downto 0);
    signal addr_to_lsq_valid: std_logic;
    signal addr_to_lsq_ready: std_logic;

    signal data_from_lsq: std_logic_vector(DATA_SIZE-1 downto 0);
    signal data_from_lsq_valid: std_logic;
    signal data_from_lsq_ready: std_logic;

    signal data_to_circuit: std_logic_vector(DATA_SIZE-1 downto 0);
    signal data_to_circuit_valid: std_logic;
    signal data_to_circuit_ready: std_logic;

begin

    addr_from_circuit <= input_addr;
    addr_from_circuit_valid <= pValidArray(1);
    readyArray(1)  <= Buffer_1_readyArray_0;

    Buffer_1: entity work.TEHB(arch) generic map (1,1,ADDRESS_SIZE,ADDRESS_SIZE)
    port map (
        clk => clk,
        rst => rst,
        dataInArray => addr_from_circuit,
        pValidArray(0) => addr_from_circuit_valid,
        readyArray(0) => Buffer_1_readyArray_0,
        nReadyArray(0) => addr_to_lsq_ready,
        validArray(0) => Buffer_1_validArray_0,
        dataOutArray => Buffer_1_dataOutArray_0
    );


    addr_to_lsq <= Buffer_1_dataOutArray_0;
    addr_to_lsq_valid <= Buffer_1_validArray_0;
    addr_to_lsq_ready <= nReadyArray(1);

    output_addr <= addr_to_lsq; -- address request goes to LSQ
    validArray(1) <= addr_to_lsq_valid;

    readyArray(0) <= data_from_lsq_ready;

    data_from_lsq <= dataInArray;
    data_from_lsq_valid <= pValidArray(0);
    data_from_lsq_ready <= Buffer_2_readyArray_0;

    dataOutArray <= Buffer_2_dataOutArray_0; -- data from LSQ to load output
    validArray(0) <=  Buffer_2_validArray_0;

    Buffer_2: entity work.TEHB(arch) generic map (1,1,DATA_SIZE,DATA_SIZE)
    port map (
        clk => clk,
        rst => rst,
        dataInArray => data_from_lsq,
        pValidArray(0) => data_from_lsq_valid,
        readyArray(0) => Buffer_2_readyArray_0,
        nReadyArray(0) => nReadyArray(0),
        validArray(0) => Buffer_2_validArray_0,
        dataOutArray => Buffer_2_dataOutArray_0
    );

        
end architecture;


library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;


entity mc_store_op is generic(  INPUTS: integer; OUTPUTS: integer; ADDRESS_SIZE : Integer;  DATA_SIZE : Integer);
port (
    clk, rst: in std_logic;


    input_addr: in std_logic_vector(ADDRESS_SIZE -1 downto 0);
    dataInArray : in std_logic_vector(DATA_SIZE -1 downto 0);

    --- interface to previous
    pValidArray : IN std_logic_vector(1 downto 0);
    readyArray : inout std_logic_vector(1 downto 0);

    ---interface to next
    dataOutArray : inout std_logic_vector(DATA_SIZE -1 downto 0);
    output_addr: inout std_logic_vector(ADDRESS_SIZE -1 downto 0);
    nReadyArray: in std_logic_vector(OUTPUTS-1 downto 0);
    validArray: inout std_logic_vector(OUTPUTS-1 downto 0));

end entity;


architecture arch of mc_store_op is
    signal single_ready: std_logic;
    signal join_valid: std_logic;
   
    begin

    join_write:   entity work.join(arch) generic map(2)
            port map(   pValidArray,  --pValidArray
                        nReadyArray(0),                  --nready                    
                        join_valid,                    --valid          
                        ReadyArray);   --readyarray 


    dataOutArray <= dataInArray; -- data to LSQ
    validArray(0) <= join_valid;


    output_addr <= input_addr; -- address to LSQ
    validArray(1) <= join_valid;
       


 end architecture;


library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;
entity MemCont is generic( DATA_SIZE: natural; ADDRESS_SIZE: natural; BB_COUNT: natural; LOAD_COUNT : natural; STORE_COUNT: natural);
port (
    rst: in std_logic;
    clk: in std_logic;
    ce: in std_logic;
    io_storeDataOut    : out std_logic_vector(DATA_SIZE-1 downto 0);
    io_storeAddrOut    : out std_logic_vector(ADDRESS_SIZE-1 downto 0);
    io_storeEnable : out std_logic;
    io_loadDataIn : in std_logic_vector(DATA_SIZE-1 downto 0);
    io_loadAddrOut : out std_logic_vector(ADDRESS_SIZE-1 downto 0);
    io_loadEnable : out std_logic;

    io_bbpValids : in std_logic_vector(BB_COUNT - 1 downto 0);
    io_bb_stCountArray: in std_logic_vector(BB_COUNT*32 - 1 downto 0);
    io_bbReadyToPrevs : inout std_logic_vector(BB_COUNT - 1 downto 0);

    io_Empty_Valid: out STD_LOGIC;
    io_Empty_Ready: in STD_LOGIC;

    io_rdPortsPrev_valid : in  std_logic_vector(LOAD_COUNT - 1 downto 0);
    io_rdPortsPrev_bits: in std_logic_vector(LOAD_COUNT*ADDRESS_SIZE -1 downto 0);
    io_rdPortsPrev_ready : inout  std_logic_vector(LOAD_COUNT - 1 downto 0);

    io_rdPortsNext_bits: inout std_logic_vector(LOAD_COUNT*DATA_SIZE -1 downto 0);
    io_rdPortsNext_valid : inout  std_logic_vector(LOAD_COUNT - 1 downto 0);
    io_rdPortsNext_ready: in  std_logic_vector(LOAD_COUNT - 1 downto 0);

    io_wrAddrPorts_valid : in  std_logic_vector(STORE_COUNT - 1 downto 0);
    io_wrAddrPorts_bits: in std_logic_vector(STORE_COUNT*ADDRESS_SIZE -1 downto 0);
    io_wrAddrPorts_ready : inout  std_logic_vector(STORE_COUNT - 1 downto 0);

    io_wrDataPorts_valid : in  std_logic_vector(STORE_COUNT - 1 downto 0);
    io_wrDataPorts_bits: in std_logic_vector(STORE_COUNT*DATA_SIZE -1 downto 0);
    io_wrDataPorts_ready : inout  std_logic_vector(STORE_COUNT - 1 downto 0)
    );

end entity;


architecture arch of MemCont is
signal counter1 : std_logic_vector(31 downto 0);
signal valid_WR: std_logic_vector(STORE_COUNT - 1 downto 0);
constant zero: std_logic_vector(BB_COUNT - 1 downto 0) := (others=>'0');
signal we: std_logic;

begin

    read_arbiter : entity work.read_memory_arbiter
        generic map(
            ARBITER_SIZE => LOAD_COUNT,
            ADDR_WIDTH   => ADDRESS_SIZE,
            DATA_WIDTH   => DATA_SIZE
        )
        port map(
            ce               => ce,
            rst              => rst,
            clk              => clk,
            pValid           => io_rdPortsPrev_valid,
            ready            => io_rdPortsPrev_ready,
            address_in       => io_rdPortsPrev_bits, -- if two address lines are presented change this to corresponding one.
            nReady           => io_rdPortsNext_ready,
            valid            => io_rdPortsNext_valid,
            data_out         => io_rdPortsNext_bits,
            read_enable      => io_loadEnable,
            read_address     => io_loadAddrOut,
            data_from_memory => io_loadDataIn
        );

    write_arbiter : entity work.write_memory_arbiter
        generic map(
            ARBITER_SIZE => STORE_COUNT,
            ADDR_WIDTH   => ADDRESS_SIZE,
            DATA_WIDTH   => DATA_SIZE
        )
        port map(
            ce             => ce,
            rst            => rst,
            clk            => clk,
            pValid         => io_wrAddrPorts_valid,
            pValidData         => io_wrDataPorts_valid,
            ready          => io_wrAddrPorts_ready,
            readyData          => io_wrDataPorts_ready,
            address_in     => io_wrAddrPorts_bits, -- if two address lines are presented change this to corresponding one.
            data_in        => io_wrDataPorts_bits,
            nReady         => (others => '1'), --for now, setting as always ready 
            valid          => valid_WR, -- unconnected
            enable         => we,
            write_address  => io_storeAddrOut,
            data_to_memory => io_storeDataOut
        );

        io_storeEnable <= we;
      
        Counter: process (CLK)
        variable counter : std_logic_vector(31 downto 0);
        begin
            if (rst = '1') then
                counter:=(31 downto 0 => '0');
              
            elsif rising_edge(CLK) then
                if ce = '1' then
                    -- increment counter by number of stores in BB
                    for I in 0 to BB_COUNT - 1 loop
                        if (io_bbpValids(I) = '1') then
                            counter:= std_logic_vector(unsigned(counter) + unsigned(io_bb_stCountArray(I*32+31 downto I*32)));
                        end if;
                    end loop;

                    -- decrement counter whenever store issued to memory
                    if (we = '1') then
                        counter:= std_logic_vector(unsigned(counter) - 1);
                    end if;

                    counter1 <= counter;
                end if;
            end if;

        end process;

        -- check if there are any outstanding store requests
        -- if not, program can terminate
        io_Empty_Valid <= '1' when (counter1 =  (31 downto 0 => '0') and (io_bbpValids(BB_COUNT -1 downto 0)  = zero))  else '0'; 

        io_bbReadyToPrevs <= (others => '1'); -- always ready to increment counter;

end architecture;