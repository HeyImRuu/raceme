# raceme
racing plugin for DiscoveryGC's FLHook
=======
Race against the clock on a specified track!
# commands
> /startrace &lt;id&gt;
> /showtracks
# config
* [config]
	* buffer - a buffer zone sphere around the start and finish positions: radius
* [data]
	* tracklist - each entry is a valid track name
* [trackname] - header of a trackname from tracklist
	* track - the track info: id,name,startx,starty,startz,finishx,finishy,finishz,system nick (eg. Br_05)
# TODO:
* allow users to create a race instance, allow others to join said instance.
* more comments