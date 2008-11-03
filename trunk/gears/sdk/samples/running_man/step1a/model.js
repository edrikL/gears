
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
