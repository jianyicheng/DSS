------------------------------------------------------
-- simple priority selector
------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity simplepriority is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        counter       : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        sel          : out integer range 0 to ARBITER_SIZE-1
    );
end entity;

architecture arch of simplepriority is

begin
    process(counter)
    begin
        sel <= 0;
        for I in 1 to ARBITER_SIZE - 1 loop
            if counter(I) = '1' then
                sel <= I;
            end if;
        end loop;
    end process;
end architecture;

------------------------------------------------------
-- priority selector
------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity priority is
    generic(
        ARBITER_SIZE : natural
    );
    port(
        req       : in  std_logic_vector(ARBITER_SIZE - 1 downto 0);
        enable    : out std_logic;
        sel       : out integer range 0 to ARBITER_SIZE-1
    );
end entity;

architecture arch of priority is

begin
    process(req)
    begin
        sel <= 0;
        enable <= '0';
        for I in 0 to ARBITER_SIZE - 1 loop
            if req(I) = '1' then
                sel <= I;
                enable <= '1';
            end if;
        end loop;
    end process;
end architecture;

------------------------------------------------------
-- combinational mux - multi-bits
------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;

entity dassArbiterMux is
    generic(
        ARBITER_SIZE : natural;
        BITWIDTH : natural
    );
    port(
        din      : in std_logic_vector(ARBITER_SIZE*BITWIDTH - 1 downto 0);
        sel      : in integer range 0 to ARBITER_SIZE-1;
        dout     : out std_logic_vector(BITWIDTH - 1 downto 0)
    );
end entity;

architecture arch of dassArbiterMux is

begin
    dout <= din(BITWIDTH*sel+BITWIDTH-1 downto BITWIDTH*sel);
end architecture;

------------------------------------------------------
-- DASS Memory Arbiter
------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.customTypes.all;
entity dassArbiter is generic( DATA_SIZE: natural; ADDRESS_SIZE: natural; MEM_COUNT: natural);
port (
    rst: in std_logic;
    clk: in std_logic;
    io_storeDataOut    : out std_logic_vector(DATA_SIZE-1 downto 0);
    io_storeAddrOut    : out std_logic_vector(ADDRESS_SIZE-1 downto 0);
    io_storeEnable : out std_logic;
    io_loadDataIn : in std_logic_vector(DATA_SIZE-1 downto 0);
    io_loadAddrOut : out std_logic_vector(ADDRESS_SIZE-1 downto 0);
    io_loadEnable : out std_logic;

    storeDataOut    : in std_logic_vector(MEM_COUNT*DATA_SIZE-1 downto 0);
    storeAddrOut    : in std_logic_vector(MEM_COUNT*ADDRESS_SIZE-1 downto 0);
    storeEnable : in std_logic_vector(MEM_COUNT-1 downto 0);
    loadDataIn : out std_logic_vector(MEM_COUNT*DATA_SIZE-1 downto 0);
    loadAddrOut : in std_logic_vector(MEM_COUNT*ADDRESS_SIZE-1 downto 0);
    loadEnable : in std_logic_vector(MEM_COUNT-1 downto 0);

    io_Empty_Valid : out std_logic;
    ready : out std_logic;

    ss_start: in std_logic_vector(MEM_COUNT-1 downto 0);
    ss_ce: out std_logic_vector(MEM_COUNT-1 downto 0);
    ss_done: in std_logic_vector(MEM_COUNT-1 downto 0)
    );
end entity;


architecture arch of dassArbiter is
-- architecture aggresive of dassArbiter is

signal loadSel : integer range 0 to MEM_COUNT-1;
signal storeSel : integer range 0 to MEM_COUNT-1;
signal toLoad : std_logic;
signal toStore : std_logic;
signal counter : std_logic_vector(MEM_COUNT-1 downto 0);
constant zero : std_logic_vector(MEM_COUNT-1 downto 0) := (others => '0');
signal loadCE : std_logic_vector(MEM_COUNT-1 downto 0);
signal storeCE : std_logic_vector(MEM_COUNT-1 downto 0);
begin
    
    io_Empty_Valid <= '1' when counter = zero else '0'; 
    process(clk)
    begin
        if rst = '1' then
            counter <= (others => '0');
        elsif rising_edge(clk) then
            for I in 0 to MEM_COUNT - 1 loop
                if ss_start(I) = '1' then
                    counter(I) <= '1';
                elsif ss_done(I) = '1' then
                    counter(I) <= '0';
                else
                    counter(I) <= counter(I);
                end if;
            end loop;
        end if;
    end process;
    
    process(toLoad, loadSel, loadEnable)
    begin
        for I in 0 to MEM_COUNT - 1 loop
            if I = loadSel then
                loadCE(I) <= '1';
            else
                loadCE(I) <= (not toLoad) or (not loadEnable(I));
            end if;
        end loop;
    end process;
    
    process(toStore, storeSel, storeEnable)
    begin
        for I in 0 to MEM_COUNT - 1 loop
            if I = storeSel then
                storeCE(I) <= '1';
            else
                storeCE(I) <= (not toStore) or (not storeEnable(I));
            end if;
        end loop;
    end process;
    
    ss_ce <= storeCE and loadCE;
    io_loadEnable <= toLoad;
    io_storeEnable <= toStore;
    
    loadSelectOp : entity work.priority(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT
        )
        port map(
            sel     => loadSel,
            req     => loadEnable,
            enable  => toLoad
        );

    loadAddrOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => ADDRESS_SIZE
        )
        port map(
            sel  => loadSel,
            din  => loadAddrOut,
            dout => io_loadAddrOut
        );
    
    storeSelectOp : entity work.priority(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT
        )
        port map(
            sel     => storeSel,
            req     => storeEnable,
            enable  => toStore
        );

    storeDataOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => DATA_SIZE
        )
        port map(
            sel  => storeSel,
            din  => storeDataOut,
            dout => io_storeDataOut
        );
    
    storeAddrOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => ADDRESS_SIZE
        )
        port map(
            sel  => storeSel,
            din  => storeAddrOut,
            dout => io_storeAddrOut
        );

    process(io_loadDataIn)
    begin
        for i in 0 to MEM_COUNT-1 loop
            loadDataIn(i*DATA_SIZE+DATA_SIZE-1 downto i*DATA_SIZE) <= io_loadDataIn;
        end loop;
    end process;

end architecture;

-- architecture arch of dassArbiter is
architecture nonaggressive of dassArbiter is
signal counter : std_logic_vector(MEM_COUNT-1 downto 0);
constant zero : std_logic_vector(MEM_COUNT-1 downto 0) := (others => '0');
signal sel : integer range 0 to MEM_COUNT-1;
signal stall : integer range 0 to 4;
constant stallCount : integer := 2;
signal toLoad : std_logic_vector(0 downto 0);
signal toStore : std_logic_vector(0 downto 0);
signal ce_buff : std_logic_vector(MEM_COUNT-1 downto 0);
begin
    io_loadEnable <= toLoad(0);
    io_storeEnable <= toStore(0);
    
    process(clk)
    variable disable : std_logic;
    begin
        if (rst = '1') then
            counter <= (others => '0');
            ce_buff <= (others => '1');
        elsif rising_edge(CLK) then
            disable := '0';
            for i in 0 to MEM_COUNT-1 loop
                if ss_start(i) = '1' then
                    ce_buff <= not ss_start;
                    ce_buff(0) <= '0';
                    ce_buff(i) <= '1';
                    counter <= (others => '0');
                    counter(i) <= '1';
                elsif ss_done(i) = '1' then
                    counter(i) <= '0';
                    ce_buff(i) <= '1';
                    ce_buff(0) <= '1';
                elsif counter = zero then
                    ce_buff(0) <= '1';
                    counter <= counter;
                end if;
            end loop;
        end if;
    end process;
    ss_ce <= ce_buff;

    -- JC: The transition between SS function and DS function takes a few cycles with a set empty_valid signal.
    -- Then the end node may think the whole simulation has finished, which is not the case.
    -- Therefore we set the empty_valid signal if the counter holds 0 for a few cycles. 2 is measured from experiment.
    process(clk)
    begin
        if (rst = '1') then
            stall <= 0;
        elsif rising_edge(CLK) then
            if counter = zero then
                if stall = stallCount then
                    stall <= stall;
                else
                    stall <= stall + 1;
                end if;
            else 
                stall <= 0;
            end if;
        end if;
    end process;
    io_Empty_Valid <= '1' when stall = stallCount else '0';
    ready <= '0' when stall = stallCount else '1';

    selectOp : entity work.simplepriority(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT
        )
        port map(
            sel      => sel,
            counter  => counter
        );


    storeDataOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => DATA_SIZE
        )
        port map(
            sel  => sel,
            din  => storeDataOut,
            dout => io_storeDataOut
        );
    
    storeAddrOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => ADDRESS_SIZE
        )
        port map(
            sel  => sel,
            din  => storeAddrOut,
            dout => io_storeAddrOut
        );
    
    storeEnOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => 1
        )
        port map(
            sel  => sel,
            din  => storeEnable,
            dout => toStore
        );
    
    loadAddrOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => ADDRESS_SIZE
        )
        port map(
            sel  => sel,
            din  => loadAddrOut,
            dout => io_loadAddrOut
        );

    loadEnOp : entity work.dassArbiterMux(arch)
        generic map(
            ARBITER_SIZE => MEM_COUNT,
            BITWIDTH   => 1
        )
        port map(
            sel  => sel,
            din  => loadEnable,
            dout => toLoad
        );

    process(io_loadDataIn)
    begin
        for i in 0 to MEM_COUNT-1 loop
            loadDataIn(i*DATA_SIZE+DATA_SIZE-1 downto i*DATA_SIZE) <= io_loadDataIn;
        end loop;
    end process;

end architecture;
