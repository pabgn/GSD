var express = require('express');
var app = express();

app.use('/static', express.static('public'));


app.get('/', function (req, res) {
	res.sendfile('./index.html');
});

app.get('/db', function (req, res) {
	res.sendfile('./database.js');
});

app.get('/robots', function (req, res) {
	res.sendfile('./robot.js');
});

app.get('/robot/:order/', function(req, res) {	
	var fs = require('fs');
	var obj;

	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);
		res.send('Server recieved order:' + req.params.order);

		if(req.params.order.substring(0,4) == "move"){
			toCoordinate = req.params.order.substring(4,10);
			obj.push({"title":"move","toCoordinate":toCoordinate});

		} if (req.params.order.substring(0,9) == "placeGood") {
			fromCoordinate = req.params.order.substring(9,14);
			toCoordinate = req.params.order.substring(15,20);
			obj.push({"title":"placeGood","fromCoordinate":fromCoordinate,"toCoordinate":toCoordinate});
		};

		fs.writeFile('./orders.js', JSON.stringify(obj), function (err) {
			if (err) return console.log(err);
			console.log(JSON.stringify(obj));
			console.log('writing to ' + './orders.js');
		});
	});
});

app.get('/order/pop', function (req, res) {
	var fs = require('fs');
	var obj;

	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);
		
		order = obj.shift();
		fs.writeFile('./orders.js', JSON.stringify(obj), function (err) {
			if (err) return console.log(err);
			console.log('Popped: "'+ JSON.stringify(order) + '" from ./orders.js');
			if (order == undefined) {
				res.send("empty");
			} else{
				res.send(JSON.stringify(order));
			};
		});
	});
});

app.listen(3000, function () {
	console.log('Warehouse webserver listening on port 3000!');
});
