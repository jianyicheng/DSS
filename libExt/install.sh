. ../env.tcl

mkdir sim
mkdir syn

$VHLS build.tcl

for file in dop fop;
do
	cp -r ${file}_top/solution1/impl/ip syn/$file
	cp -r ${file}_top/solution1/impl/ip/hdl/ip/*.vhd sim/
	cp -r ${file}_top/solution1/impl/ip/hdl/vhdl/*.vhd sim/
	rm sim/$file.vhd
	#rm -r ${file}_top
done

echo "library generation succeeded!"
