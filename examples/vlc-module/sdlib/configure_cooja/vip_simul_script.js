TIMEOUT(10000);
log.log("first simulation message at time : " + time + "\n");
while (true) {
  YIELD(); /* wait for another mote output */
}