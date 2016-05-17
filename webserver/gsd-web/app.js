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

app.listen(3000, function () {
  console.log('Example app listening on port 3000!');
});
