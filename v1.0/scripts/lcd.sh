#27/10/26 21:01:27 update, new version
#FIXED: fix right part of tft

DEV=/dev/spidev1.0
[ -c $DEV ] || { echo "нет $DEV"; return 1; }
[ -d /sys/class/gpio/gpio12 ] || echo 12 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio12/direction
cmd() { echo 0 > /sys/class/gpio/gpio12/value; printf "$1" > $DEV; echo 1 > /sys/class/gpio/gpio12/value; }
dat() { printf "$1" > $DEV; }
init() {
  cmd '\001'; sleep 1
  cmd '\021'; sleep 1
  cmd '\072'; dat '\125'
  cmd '\066'; dat '\000'
  cmd '\051'
}
fill() {
  printf "$1" > /tmp/p
  for i in 1 2 3 4 5 6 7 8 9 10 11; do cat /tmp/p /tmp/p > /tmp/q; mv /tmp/q /tmp/p; done
  cmd '\052'; dat '\000\000\000\357'
  cmd '\053'; dat '\000\000\001\077'
  cmd '\054'
  n=0; while [ $n -lt 38 ]; do dd if=/tmp/p of=$DEV bs=4096 2>/dev/null; n=$((n+1)); done
}
init2() {
  cmd '\001'; sleep 1
  cmd '\050'
  cmd '\317'; dat '\000\203\060'
  cmd '\355'; dat '\144\003\022\201'
  cmd '\350'; dat '\205\001\171'
  cmd '\313'; dat '\071\054\000\064\002'
  cmd '\367'; dat '\040'
  cmd '\352'; dat '\000\000'
  cmd '\300'; dat '\046'
  cmd '\301'; dat '\021'
  cmd '\305'; dat '\065\076'
  cmd '\307'; dat '\276'
  cmd '\072'; dat '\125'
  cmd '\066'; dat '\000'
  cmd '\261'; dat '\000\033'
  cmd '\046'; dat '\001'
  cmd '\267'; dat '\007'
  cmd '\266'; dat '\012\202\047\000'
  cmd '\021'; sleep 1
  cmd '\051'; sleep 1
}
