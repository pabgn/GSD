var express = require('express');
var app = express();
var gecodeRunning = false;

app.use('/static', express.static('public'));


app.get('/', function (req, res) {
	res.sendfile('./index.html');
});

app.get('/db', function (req, res) {
	res.sendfile('./sections.js');
});

app.get('/sensors', function (req, res) {
	res.sendfile('./sensors.js');
});

app.get('/robot', function (req, res) {
	res.sendfile('./robot.js');
});

app.get('/robot/:order/', function(req, res) {	
	var fs = require('fs');
	var obj;
	var to_xCoord;
	var to_yCoord;
	var from_xCoord;
	var from_yCoord;

	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);
		stringOrder = req.params.order;
		res.send('Server recieved order:' + stringOrder);

		if(stringOrder.substring(0,4) == "move"){
			to_xCoord = stringOrder.substring(5,6);
			to_yCoord = stringOrder.substring(7,8);
			obj.push({"job":"move","to":{"x_coord":to_xCoord,"y_coord":to_yCoord}});

		} if (stringOrder.substring(0,9) == "placeGood") {
			from_xCoord = stringOrder.substring(10,11);
			from_yCoord = stringOrder.substring(12,13);
			to_xCoord = stringOrder.substring(15,16);
			to_yCoord = stringOrder.substring(17,18);		
			obj.push({"job":"placeGood","from":{"x_coord":from_xCoord,"y_coord":from_yCoord},"to":{"x_coord":to_xCoord,"y_coord":to_yCoord}});
		} if (stringOrder.substring(0,10) == "removeGood") {
			from_xCoord = stringOrder.substring(11,12);
			from_yCoord = stringOrder.substring(13,14);
			obj.push({"job":"remove","from":{"x_coord":from_xCoord,"y_coord":from_yCoord}});						
		};

		fs.writeFile('./orders.js', JSON.stringify(obj, null,1), function (err) {
			if (err) return console.log(err);
			console.log('writing to ' + './orders.js');
		});
	});
});

app.get('/add/:name/:temp_min/:temp_max/:light_min/:light_max', function (req, res) {
	var fs = require('fs');
	var obj;
	good = req.params;

	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);

		res.send('Server recieved order: add('+good.name+'\n,Temp min: '+good.temp_min+', max: '+good.temp_max+'\nLight min: '+good.light_min+', max: '+good.light_max+')');
		obj.push({"job":"add",
					"good":{
						"name": good.name,
						"desiredTemperature":{"min": good.temp_min,"max": good.temp_max},
        				"desiredLighting":{"min":good.light_min, "max":good.light_max}        
        			}
        		});						
		
		fs.writeFile('./orders.js', JSON.stringify(obj, null,1), function (err) {
			if (err) return console.log(err);
			console.log('writing to ' + './orders.js');
		});
	});
});


function sendOrderToGecode(){
	var fs = require('fs');
	var obj;
	var orderToSend;
	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);

		if (obj[0] == undefined) {

		}else {
			orderToSend = obj.shift();			
		};

		//send the order to gecode here

		fs.writeFile('./orders.js', JSON.stringify(obj, null,1), function (err) {
			if (err) return console.log(err);
			console.log('writing to ' + './orders.js');
		});
	});
};

app.listen(3000, function () {
	console.log('Warehouse webserver listening on port 3000!');
});
