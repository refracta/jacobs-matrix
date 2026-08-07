export TCODDIR=$(dirname $0)
export LD_LIBRARY_PATH=../lib/libtcod-1.4.0:${LD_LIBRARY_PATH}
cd ${TCODDIR} && ./jacob_bin $*
