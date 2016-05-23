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

app.get('/robot/:order', function(req, res) {	
	var fs = require('fs');
	var obj;

	fs.readFile('./orders.js', 'utf8', function (err, data) {
		if (err) throw err;
		obj = JSON.parse(data);
		console.log("Recieved coordinate: "+req.params.order);
		res.send('Server recieved order:' + req.params.order);

		if(req.params.order.substring(0,4) == "move"){
			x_coord = req.params.order.substring(5,6);
			y_coord = req.params.order.substring(7,8);
			obj.push({"title":"move","x_coordinate":x_coord,"y_coordinate":y_coord});

		} if (req.params.order.substring(0,7) == "getGood") {
			x_coord = req.params.order.substring(8,9);
			y_coord = req.params.order.substring(10,11);
			obj.push({"title":"getGood","x_coordinate":x_coord,"y_coordinate":y_coord});

	} if (req.params.order.substring(0,9) == "placeGood") {
		//x_coord = req.params.order.substring(10,11);
		//y_coord = req.params.order.substring(12,13);
		//obj.orders.push({"title":"placeGood", "good":{"name":"coke"},"x_coordinate":x_coord,"y_coordinate":y_coord});
	};
	
	fs.writeFile('./orders.js', JSON.stringify(obj), function (err) {
	  	if (err) return console.log(err);
	  	console.log(JSON.stringify(obj));
	  	console.log('writing to ' + './orders.js');
	  });
});
});

app.listen(3000, function () {
	console.log('Example app listening on port 3000!');
});
