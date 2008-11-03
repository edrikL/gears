var global = new Object();

/*
 * Main function
 */
function main() {
  global.startTime = null;
  updateTime();
}

/*
 * Timers functions
 */

function startRun() {
  global.updateInterval = setInterval("updateTime()", 1000);
  global.startTime = new Date();
}

function stopRun() {
  clearInterval(global.updateInterval);
}

function resetRun() {
  stopRun();
  global.startTime = null;
  updateTime();
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
