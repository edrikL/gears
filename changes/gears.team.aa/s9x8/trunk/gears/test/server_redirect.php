<?php 
// Copyright 2006, Google Inc.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Google Inc. nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Usage: serverRedirect.php?location=url[&full=notused]
// Generates an HTTP 302 response with the location header set to 'url'.
// If 'full' is specified, a full location url will be formed.

// Returns a full url given an absolute base and a relative url
function InternetCombineUrl($absolute, $relative) {
  $p = parse_url($relative);
  if($p["scheme"])return $relative;
  
  extract(parse_url($absolute));
  
  $path = dirname($path); 

  if($relative{0} == '/') {
    $cparts = array_filter(explode("/", $relative));
  }
  else {
    $aparts = array_filter(explode("/", $path));
    $rparts = array_filter(explode("/", $relative));
    $cparts = array_merge($aparts, $rparts);
    foreach($cparts as $i => $part) {
      if($part == '.') {
        $cparts[$i] = null;
      }
      if($part == '..') {
        $cparts[$i - 1] = null;
        $cparts[$i] = null;
      }
    }
    $cparts = array_filter($cparts);
  }
  $path = implode("/", $cparts);
  $url = "";
  if($scheme) {
    $url = "$scheme://";
  }
  if($user) {
    $url .= "$user";
    if($pass) {
      $url .= ":$pass";
    }
    $url .= "@";
  }
  if($host) {
    $url .= "$host/";
  }
  $url .= $path;
  return $url;
}

$location = $_GET["location"];
if ($location) {
  $location_value = $location;
  if (isset($_GET["full"])) {
    $base = $_SERVER["SCRIPT_URI"];
    $location_value = InternetCombineUrl($base, $location);
  }
  header("HTTP/1.0 302 Found");
  header("Location: $location_value");
} else {
  echo "missing location parameter";
}
?>
