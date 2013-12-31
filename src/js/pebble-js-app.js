Pebble.addEventListener("ready", function (e) {
    console.log("Hello, world from bcdface js...");
});

Pebble.addEventListener("showConfiguration", function (e) {
    Pebble.openURL();
});
