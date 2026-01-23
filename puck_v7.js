LED1.write(0);
LED2.write(0);
LED3.write(0);

NRF.setTxPower(-8);

var LOW_BATT_LIMIT = 25; // %

function readAndSend() {

  // Enable ACC + GYRO together (CORRECT WAY)
  Puck.accelOn({ gyro:true });

  // Give IMU time to settle
  setTimeout(function () {

    function onAccel(a) {

      var temp = Math.round(E.getTemperature());
      var batt = Math.round(Puck.getBatteryPercentage());

      var buf = new Uint8Array(17);
      var dv  = new DataView(buf.buffer);

      // Header
      buf[0] = 0xAA;
      buf[1] = 0x55;

      // ACC
      dv.setInt16(2,  a.acc.x | 0, true);
      dv.setInt16(4,  a.acc.y | 0, true);
      dv.setInt16(6,  a.acc.z | 0, true);

      // GYRO ✅ now works
      dv.setInt16(8,  a.gyro.x | 0, true);
      dv.setInt16(10, a.gyro.y | 0, true);
      dv.setInt16(12, a.gyro.z | 0, true);

      // TEMP + BATT
      dv.setInt8 (14, temp);
      dv.setUint8(15, batt);

      // CRC (bytes 2–15)
      var cs = 0;
      for (var i = 2; i < 16; i++) cs ^= buf[i];
      buf[16] = cs;

      // Send packet
      Bluetooth.write(buf);

      // Cleanup
      Puck.removeListener("accel", onAccel);
      Puck.accelOff();

      // Low battery → deep sleep
      if (batt < LOW_BATT_LIMIT) {
        setTimeout(function () {
          NRF.sleep();
        }, 200);
      }
    }

    Puck.on("accel", onAccel);

  }, 200);
}

clearInterval();
setInterval(readAndSend, 5*60 * 1000);
readAndSend();

//save();  // run once, then comment

