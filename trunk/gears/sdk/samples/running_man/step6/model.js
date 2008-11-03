var global = new Object();

/*
 * Main function
 */
function main() {
  global.startTime = null;
  global.geo = google.gears.factory.create('beta.geolocation');
  updateTime();
  global.siteUrl = "http://code.google.com/apis/gears/samples/running_man/step6/index.html";
  initDB();
  installShortcut();
}

/*
 * Initialize the Database
 */
function initDB() {
  global.db = google.gears.factory.create('beta.database');
  global.db.open('stopwatch');
  global.db.execute('CREATE TABLE IF NOT EXISTS Preferences ' +
             '(Name text, Value int)');
  global.db.execute('CREATE TABLE IF NOT EXISTS Times ' +
             '(StartDate int, StopDate int, Description text)');
  global.db.execute('CREATE TABLE IF NOT EXISTS Positions ' +
             '(TimeID int, Date int, Latitude float, Longitude float, ' +
             'Accuracy float, Altitude float, AltitudeAccuracy float)');
}

function getPreference(name) {
  var result = false;
  var rs = global.db.execute('SELECT Value FROM Preferences WHERE Name = (?)', [name]);

  if (rs.isValidRow()) {
    result = rs.field(0);
  }
  rs.close();
  return result;
}

function setPreference(name, value) {
  global.db.execute('INSERT INTO Preferences VALUES (?, ?)', [name, value]);
}


/*
 * Install shortcut
 */
function installShortcut() {
  if (getPreference('Shortcut') == false) {
    var desktop = google.gears.factory.create('beta.desktop');
    desktop.createShortcut("RunningMan", global.siteUrl,
      { "48x48" : "../images/icon.png" }, "RunningMan Step6");
    setPreference('Shortcut', true);
  }
}

/*
 * Timers functions
 */

function startRun() {
  global.updateInterval = setInterval("updateTime()", 1000);
  global.startTime = new Date();
  var time = global.startTime.getTime();
  global.db.execute('INSERT INTO Times (StartDate) VALUES (?)', [time]);
  global.currentTimeID = global.db.lastInsertRowId;
  global.currentGeoWatcher = global.geo.watchPosition(function (position) {
    global.db.execute('INSERT INTO Positions (TimeID, Date, Latitude, ' +
               'Longitude, Accuracy, Altitude, AltitudeAccuracy) ' +
               'VALUES (?, ?, ?, ?, ?, ?, ?)',
               [global.currentTimeID, position.timestamp.getTime(),
               position.latitude, position.longitude, position.accuracy,
               position.altitude, position.altitudeAccuracy]);
    }, null, { "enableHighAccuracy" : true});
}

function stopRun() {
  clearInterval(global.updateInterval);
  var stopDate = new Date();
  var time = stopDate.getTime();
  global.db.execute('UPDATE Times SET StopDate = (?) ' +
                 'WHERE ROWID = (?)', [time, global.currentTimeID]);
  if (global.currentGeoWatcher != null) {
    global.geo.clearWatch(global.currentGeoWatcher);
    global.currentGeoWatcher = null;
  }
}

function resetRun() {
  stopRun();
  global.startTime = null;
  updateTime();
}

/*
 * Show journeys
 */
function on_journeys() {
  var rows = 0;
  var html = "";

  var rs = global.db.execute('SELECT ROWID, Description FROM Times');

  while (rs.isValidRow()) {
    var rowID = rs.field(0);
    var description = rs.field(1);

   if (description == null) {
      description = createDescription(rowID);
   }

    var rowType;
    if (rows % 2 == 0) {
      rowType = "rowA";
    } else {
      rowType = "rowB";
    }
    rows++;

    html += "<div class='" + rowType + "'>";
    html += "<div class='rowDesc'>" + description + "</div>";
    html += "<div class='rowImg' onclick='deleteRecord(" + rowID + ")'>";
    html += "<img src='../images/delete.png'></div";
    html += "</div>";

    rs.next();
  }

  if (html != "") {
    var elem = document.getElementById('journeysContent');
    elem.innerHTML = html;
  }
}

/*
 * Utility functions to compute the distance between
 * two coordinates
 */
Number.prototype.toRad = function() {  // convert degrees to radians
  return this * Math.PI / 180;
}

function haversineDistance(lat1, long1, lat2, long2) {
  // from http://www.movable-type.co.uk/scripts/latlong.html
  var R = 6371; // km
  var dLat = (lat2-lat1).toRad();
  var dLon = (long2-long1).toRad();
  var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
          Math.cos(lat1.toRad()) * Math.cos(lat2.toRad()) *
                  Math.sin(dLon/2) * Math.sin(dLon/2);
  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
  var d = R * c;
  return d;
}

/*
 * Returns informations based on the associated positions
 */
function positionInformation(rowID) {
  var distance = 0;
  var prevLat = null;
  var prevLon = null;
  var firstTime = 0;
  var lastTime = 0;
  var nbPositions = 0;
  var rs = global.db.execute('SELECT Latitude, Longitude, Date ' +
                             'FROM Positions WHERE TimeID = (?) ' +
                             'ORDER BY Date', [rowID]);

  while (rs.isValidRow()) {
    nbPositions++;
    var lat = rs.field(0);
    var lon = rs.field(1);
    var date = rs.field(2);

    if (firstTime != 0) {
      distance += haversineDistance(prevLat, prevLon, lat, lon);
    } else {
      firstTime = date;
    }
    prevLat = lat;
    prevLon = lon;
    lastTime = date;
    rs.next();
  }

  var secTime = (lastTime - firstTime) / 1000;
  var averageSpeed = ((distance * 3600) / secTime);
  var roundedDistance = (((distance*1000)|0)/1000);
  var roundedSpeed = ((averageSpeed*1000)|0)/1000;

  var description = " (" + roundedDistance + " km)";
  description += "<br>Average speed: " + roundedSpeed + " km/h";
  description += "<br>" + nbPositions + " positions saved";

  return description;
}

/*
 * Create the text displayed in the journeys pane for one record
 */
function createDescription(rowID) {
  var description = "";
  var rs = global.db.execute('SELECT StartDate, StopDate FROM Times ' +
                             'WHERE ROWID = (?)', [rowID]);

  if (rs.isValidRow()) {
    var sDate = rs.field(0);
    var eDate = rs.field(1);

    var time = (((eDate - sDate)/1000)|0); // elapsed time in seconds
    var startDate = new Date();
    startDate.setTime(sDate);

    description = startDate.toLocaleDateString() + " -- ";
    description += formatTime(time);
    description += positionInformation(rowID);

    global.db.execute('UPDATE Times SET Description = (?) ' +
                      'WHERE ROWID = (?)', [description, rowID]);
  }

  return description;
}

/*
 * Delete a recorded time
 */
function deleteRecord(rowID) {
  var answer = confirm("Delete this run?");
  if (answer) {
    global.db.execute('DELETE FROM Times WHERE ROWID = (?)', [rowID]);
    global.db.execute('DELETE FROM Positions where TimeID = (?)', [rowID]);
    go('journeys');
  }
}

/*
 * display stopwatch time value
 */
function updateTime() {
  var time = 0;
  if (global.startTime != null) {
    time = (new Date()).getTime() - global.startTime;
    time = (time/1000)|0;
  }
  var timeDiv = document.getElementById("timeDisplay");
  timeDiv.innerHTML = formatTime(time);
}

/*
 * Format a time value given in seconds
 */
function formatTime(aTime) {
  var seconds = aTime % 60;
  var minutes = ((aTime / 60) |0) % 60;
  var hours = (aTime / 3600) |0;
  var time = "0";
  if (seconds > 0) {
    time = seconds + " s";
  }
  if (minutes > 0) {
    time = minutes + " min " + time;
  }
  if (hours > 0) {
    time = hours + " h " + time;
  }
  return time;
}

/*
 * Navigation functions
 */

function go(name) {
  hideAllScreens();
  var functionName = "on_" + name;
  showDiv(name);
  if (window[functionName] != null) {
    window[functionName]();
  }
}

function showDiv(name) {
  var elem = document.getElementById(name);
  if (elem) {
    elem.style.display="block";
  }
}

function hideDiv(name) {
  var elem = document.getElementById(name);
  if (elem) {
    elem.style.display="none";
  }
}

function hideAllScreens() {
  hideDiv('mainScreen');
  hideDiv('watch');
  hideDiv('journeys');
}
