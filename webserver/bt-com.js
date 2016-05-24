var btSerial = new (require('bluetooth-serial-port')).BluetoothSerialPort();

	btSerial.connect("00-16-53-4b-c6-7b", 1, function() {
	   console.log('connected');
	            


	   btSerial.on('data', function(buffer) {
		            
	                console.log(buffer.toString());
	     });
	            
	 }, function () {
	            console.log('cannot connect');
	 });
