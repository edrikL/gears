# Contributing to Gears #

Gears is [open source software](http://en.wikipedia.org/wiki/Open_source). At its most basic, that means that you can [get the source code](http://code.google.com/p/google-gears/source) for Gears and use it for, [basically](http://www.opensource.org/licenses/bsd-license.php), whatever you want. But Gears is also developed openly. Anyone can sign up to our [development mailing list](http://groups.google.com/group/google-gears-eng) to listen in on what we're working on or make suggestions. Anyone can [file bugs](http://code.google.com/p/google-gears/issues/list), or even submit changes to the Gears source code.

Sound like fun? This document covers the basics of how to get involved and start contributing to Gears.


# The Gears Mission #

The development of Gears is motivated by and organized around a simple desire:

**The Gears mission is to add new capabilities to web browsers, enabling developers to create better applications.**

This may sound broad, especially compared to the relatively few modules we provide today. But we have big dreams for Gears. If there is a capability that is missing from web browsers which would enable better applications, we want to provide that capability as a Gears module.


# Join the Community #

The Gears community exists primarily through our mailing lists ([google-gears](http://groups.google.com/group/google-gears) and [google-gears-eng](http://groups.google.com/group/google-gears-eng)), the [issue tracker](http://code.google.com/p/google-gears/issues/list), and our [IRC](http://en.wikipedia.org/wiki/Internet_Relay_Chat) channel, `#gears` on `irc.freenode.net`.

Everyone is encouraged to contribute to the discussion, and you can help us stay effective by following these guidelines:

  * **Please be friendly:** Showing courtesy and respect to others is a vital part of any project, but it is especially important in open source projects where communication is often not face-to-face. Misunderstandings are easy and a negative environment can easily ruin an otherwise successful project. Please go out of your way to show everyone respect, and to assume no disrespect from those you interact with.

  * **Stay on topic:** Different forums in the Gears community are optimized for different types of discussion. The [google-gears](http://groups.google.com/group/google-gears) mailing list is the place for general discussion about Gears, and for questions on using Gears to develop applications. Discussion about development of Gears itself should happen on [google-gears-eng](http://groups.google.com/group/google-gears-eng). The `#gears` IRC channel is a hangout for anyone interested in Gears, and can be a convenient place to get quick questions answered. For longer or more detailed discussion, please use one of the two mailing lists instead.


# Filing Bugs #

Anyone can file bugs or feature requests in the Gears [issue tracker](http://code.google.com/p/google-gears/issues/list). Please search carefully before creating an issue to keep the number of duplicates manageable. Also, try to provide as much information as possible when creating an issue so that a developer can easily reproduce and debug it.


# Submitting Changes #

The Gears team would love help with changes to the code. To keep things efficient, we request that contributors follow the system below:

## Submit a CLA ##

Before we can accept a patch from you, you must sign a Contributor License Agreement (CLA). The CLA protects you and us.

  * If you are an individual writing original source code and you're sure you own the intellectual property, then you'll need to sign an [individual CLA](http://code.google.com/legal/individual-cla-v1.0.html).
  * If you work for a company that wants to allow you to contribute your work to Gears, then you'll need to sign a [corporate CLA](http://code.google.com/legal/corporate-cla-v1.0.html).

Follow either of the two links above to access the appropriate CLA and instructions for how to sign and return it.


## Choose a Bug ##

Gears development is driven primarily from the issue tracker. Each issue has a version assigned to it, which is the version of Gears that we want to include the resolution to that issue in. Generally speaking, issues assigned to lower versions are more important, but sometimes larger issues are scheduled for further in the future.

If the project you want to work on does not have a bug, please create one. If the project already has a bug, assign the bug to yourself. If the bug is already assigned to someone else, update the bug or send mail to that person to see how you can help.


## Communicate your plans ##

Once you decide to start work on a bug, please communicate your plans early and often. You should update the bug with your ideas for addressing the issue. Depending on the size of the change, you may want to send intermediate patches to the mailing list for feedback. Do not go away for weeks at a time and then come back with a huge patch all at once. These patches will probably not get approved.


## Get the code ##

Gears can be checked out anonymously using any SVN client. See the [instructions](http://code.google.com/p/google-gears/source) for more information.

## Edit ##

Before making changes to the Gears codebase it may benefit you to read up on the [conventions used in our code](GearsCodingGuidelines.md).

It's also recommend that you review the [Gears Coding Guidelines](http://code.google.com/p/google-gears/wiki/GearsCodingGuidelines) page.

## Compile ##

There are wiki pages explaining how to compile for [Windows](http://code.google.com/p/google-gears/wiki/BuildingGoogleGearsOnWindows) and [Safari](http://code.google.com/p/google-gears/wiki/BuildingGearsForSafari). Builds for Firefox on mac and linux can usually be built by simply typing `make` on the command line, but sometimes linux builds do not work. Perhaps you can help [make this better](http://code.google.com/p/google-gears/issues/detail?id=68).

More info on hacking the Gears codebase can be found [here](HackingGears.md).


## Test ##

Unit tests are critical to keeping the quality of Gears high. Since Gears is primarily exposed as JavaScript, our unit tests are also implemented in JavaScript. You can learn how to get started writing tests for Gears by reading gears/test2/README.txt.


## Send Your Patch ##

Once your change is complete and you have implemented tests, create a patch file by running `svn diff` on your client. Attach the patch file to your bug and update the bug to request review of the patch. A Gears team member will likely request one or more changes to the patch. Once the review is complete, the Gears team member will commit your patch to the Gears codebase.