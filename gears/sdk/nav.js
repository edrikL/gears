// Copyright 2007, Google Inc.
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

// This file outputs the navigation so that we don't have to change it in every
// HTML file. 

// TODO: This menu is not accessible. Replace with HTML-based solution.

var nav = [
  { 
    title: "Google Gears Home", 
    id: "home",
    url: "index.html" 
  },
  { 
    title: "Install", 
    id: "install",
    url: "install.html" 
  },
  { 
    title: "Sample Applications", 
    id: "sample", 
    url: "sample.html" 
  },
  { 
    title: "Developer's Guide",
    submenu: [
      { 
        title: "Getting Started",
        id: "design",
        url: "design.html" 
      },
      {
        title: "Tutorial",
        id: "tutorial",
        url: "tutorial.html" 
      },
      {
        title: "Architecture",
        id: "architecture",
        url: "architecture.html" 
      },	  
      {
        title: "Security",
        id: "security",
        url: "security.html" 
      },	  
      { 
        title: "Resources and Tools",
        id: "tools",
        url: "tools.html" 
      } 
    ] 
  },
  { 
    title: "API Reference",
    id: "api",
    submenu: [
      { 
        title: "Summary",
        id: "api_summary",
        url: "api_summary.html" 
      },
      { 
        title: "Factory",
        id: "api_factory",
        url: "api_factory.html" 
      },
      { 
        title: "Database",
        id: "api_database",
        url: "api_database.html" 
      },
      { 
        title: "HttpRequest",
        id: "api_httprequest",
        url: "api_httprequest.html" 
      },      
      { 
        title: "LocalServer",
        id: "api_localserver",
        url: "api_localserver.html" 
      },
      { 
        title: "Timer",
        id: "api_timer",
        url: "api_timer.html" 
      },
      { 
        title: "WorkerPool",
        id: "api_workerpool",
        url: "api_workerpool.html" 
      } 
    ] 
  }
];

var resources = [
  { 
    title: "Resources",
    submenu: [
      { 
        title: "FAQ",
        url: "http://code.google.com/support/bin/topic.py?topic=11628" 
      },
      { 
        title: "Gears Blog",
        url: "http://gearsblog.blogspot.com/" 
      },
      { 
        title: "Developer Forum",
        url: "http://groups.google.com/group/google-gears/" 
      },
      { 
        title: "Contributor Site",
        url: "http://code.google.com/p/google-gears/" 
      } 
    ] 
  } 
];


// Now write out the nav html
function buildMenu(items) {
  var ul = document.createElement("ul");

  for (var i = 0; i < items.length; i++) {
    ul.appendChild(buildItem(items[i]));
  }

  return ul;
}

function buildItem(item) {
  var li = document.createElement("li");
  var elm;

  if (item.url) {
    elm = document.createElement("a");
    elm.href = item.url;

    if (item.id) {
      elm.id = item.id + "_link";
    }
  } else {
    elm = document.createElement("h1");
  }

  li.appendChild(elm);
  elm.appendChild(document.createTextNode(item.title));

  if (item.submenu) {
    li.appendChild(buildMenu(item.submenu));
  }

  return li;
}

document.write('<ul>' + buildMenu(nav).innerHTML + '</ul>');
document.write('<div class="line"></div>');
document.write('<ul>' + buildMenu(resources).innerHTML + '</ul>');

// search form
document.write(
'<div id="search">' + 
  '<form action="http://www.google.com/search" method="get">' +
    '<div>' +
      '<input type="hidden" name="domains" value="code.google.com"/>' +
      '<input type="hidden" name="sitesearch" value="code.google.com"/>' +
      '<div class="header">Search this site:</div>' +
      '<div class="input"><input name="q" size="10"/></div>' +
      '<div class="button"><input type="submit" value="Search"/></div>' +
    '</div>' +
  '</form>' +
'</div>');

// Area for spitting out debug info.
if(0) document.write('<div id="log"></div>');
