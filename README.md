# PebbleIFTTT
If This Then That (IFTTT) Client For Pebble Time + Pebble Time Round

ifttt.com is a website that allows you to connect up things in a simple way. For example if the weather forcast is for rain then send a notification to your phone (+ Pebble). This app allows you to trigger ITFFF recipes from your Pebble Time. For example my use case was to turn my LightwaveRF house lights on and off from a menu on my Pebble Time.

This works by using the Maker channel on IFTTT (https://ifttt.com/maker). Set up as many named "Recieve a web request" triggers that you want. Then use this app's configuration page (in the Pebble Time app) to set up a menu on your Pebble which you can use to activate these events. Simpliest configuration would be something like...

```
Item Name:MyMakerEventName
Next Item:OtherMakerEventName
```

But you can also set up sub menus by putting the menu name, followed by the event pairs and then a blank line to complete that submenu. In this way you can build up as complex a nested menu as you like.

```
Sitting Room Lights
On:sittingroomlightson
Off:sittingroomlightsoff

```

I intend to do a short YouTube video which I will add here in due course.
