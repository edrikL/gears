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

// probably more stuff like bind() should go in here, but meh.

if (typeof console == 'undefined') {
  var console = {
    log: function() {
//	  var msg = [];
//	  for(var i = 0; i < arguments.length; i++) {
//		msg.push(arguments[i]);
//	  }
//	  alert(msg.join('\n'));
	},
    warn: function() {
//	  var msg = [];
//	  for(var i = 0; i < arguments.length; i++) {
//		msg.push(arguments[i]);
//	  }
//	  alert(msg.join('\n'));
    }
  };
 }

// standard DOM manipulation
var DOM = new function() {
	this.gel = document.getElementById ? function(el) {
	    return document.getElementById(el);
	} : function(el) {
	    return document.all[el];
	}
	
	this.getElementsByTagName = document.getElementsByTagName ? function(el, tagName) {
		if (el.getElementsByTagName) {
			return el.getElementsByTagName(tagName);
		}
		var matches = [];
		var candidates = document.getElementsByTagName(tagName);
		for(var i = 0; i < candidates.length; i++) {
			var one = candidates[i];
			traverseUp(el, one) && matches.push(one);
		}
		return matches;
	} : function(el, tagName) {
		// nasty one-by-one traversal
		var all = document.all;
		var matches = [];
		for(var i = 0; i < all.length; i++) {
			var one = all[i];
			one.nodeName == tagName && traverseUp(el, one) && matches.push(one);
		}
		return matches;
	}
	
	this.listen = function(elm, ev, fn) {
	  if (elm.addEventListener) {
	    elm.addEventListener(ev, fn, false);
	  } else {
		if (elm.attachEvent) {
		    elm.attachEvent('on' + ev, fn);
		}
		else {
			elm['on' + ev] = fn;
		}
	  }
	}
	
	this.is_pie = navigator.appName.indexOf('Mobile') >= 0;
	
	this.is_ie = navigator.appName == 'Microsoft Internet Explorer';

	function traverseUp(el, one) {
		var descendant = one === el;
		while((one = one.parentNode) && !descendant) {
			descendant = one === el;
		}
		return descendant;
	}	
};
