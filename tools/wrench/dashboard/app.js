const express       = require("express"),
      app           = express(),
      port          = 3000;

app.set("view engine", "ejs");

app.get("/", function(req, res) {
  res.render("index");
});

app.listen(port, function() {
  console.log(`WRENCH dashboard is running on http://localhost:${port}!`);
});
