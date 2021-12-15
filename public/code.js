function loadModule(moduleName, callback) {
  require("http").get("https://plast-technologies-default-rtdb.europe-west1.firebasedatabase.app/elements.json", function(res) {
    var contents = "";
    res.on('data', function(data) { contents += data; });
    res.on('close', function() {
      Modules.addCached(moduleName, contents);
      if (callback) callback();
    });
  }).on('error', function(e) {
    console.log("ERROR", e);
  });
}