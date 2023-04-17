library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity BlockDesign_wrapper is
end BlockDesign_wrapper;

architecture STRUCTURE of BlockDesign_wrapper is
  component BlockDesign is
  end component BlockDesign;
begin
BlockDesign_i: component BlockDesign
 ;
end STRUCTURE;
