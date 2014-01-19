// (called <manufacturer><device>.init(ID,debugging) and <manufacturer><device>.shutdown() )
function DNHS5500() {}
DNHS5500.debug=3;

DNHS5500.init = function init(id,debug) {
    engine.connectControl("[Channel1]","track_samples","DNHS5500.loadLights");
    engine.connectControl("[Channel2]","track_samples","DNHS5500.loadLights");
    //engine.connectControl("[Channel1]","bpm","DNHS5500.bpm");
    //engine.connectControl("[Channel2]","bpm","DNHS5500.bpm");
    //engine.beginTimer(20,"DNHS5500.bpm");
    //engine.connectControl("[Channel1]","keylock","DNHS5500.keylocklights");
    engine.connectControl("[Channel1]","rate","DNHS5500.rateDisplay");
    engine.connectControl("[Channel2]","rate","DNHS5500.rateDisplay");
}
DNHS5500.shutdown = function shutdown() {}

// The button that enables/disables scratching
//DNHS5500.wheelTouch = function (channel, control, value, status) {
   // if ((status & 0x04) == 0x90) {    // If button down
    //if ((status & 0xF0) == 0x90) {    // If button down
  //if (value == 0x7F) {  // Some wheels send 0x90 on press and release, so you need to check the value
        //var alpha = 1.0/8;
        //var beta = alpha/32;
        //engine.scratchEnable(DNHS5500.currentDeck, 128, 33+1/3, alpha, beta);
        //engine.scratchEnable(0, 128, 33+1/3, alpha, beta);
        // Keep track of whether we're scratching on this virtual deck - for v1.10.x or below
        // MyController.scratching[MyController.currentDeck] = true;
    //}
    //else {    // If button up
        //engine.scratchDisable(DNHS5500.currentDeck);
        //MyController.scratching[MyController.currentDeck] = false;  // Only for v1.10.x and below
    //}
//}
// The wheel that actually controls the scratching
//DNHS5500.wheelTurn = function (channel, control, value, status) {
//print("channel="+channel+"control="+control+"value="+value+"status="+status);    // See if we're scratching. If not, skip this.
    //if (!engine.isScratching(DNHS5500.currentDeck)) return; // for 1.11.0 and above
    ////if (!MyController.scratching[MyController.currentDeck]) return; // for 1.10.x and below

    // --- Choose only one of the following!

    //// A: For a control that centers on 0:    var newValue;
    //if (value-64 > 0) newValue = value-128;
    //else newValue = value;

    // B: For a control that centers on 0x40 (64):
    //var newValue=(value-64);

    // --- End choice

    // In either case, register the movement
    //engine.scratchTick(DNHS5500.currentDeck,newValue);
//}

var duration = engine.getValue("[Channel1]","duration");
print ("DURATION"+duration /60);
//midi.sendShortMsg(0xB0,0x72,44); // little pitch numbers
//var BPMRound = String.format("%.2g%n",engine.getValue("[Channel1]","bpm"));
//print(engine.getValue("[Channel1]","rate"));


DNHS5500.loadLights = function (value,group) {
	if (group.charAt(8) == 1) {
    	var channel = "0xB0";
	}else{
    	var channel = "0xB1";
	}
	print("channel= "+channel);
	if (value !== 0) {
    	midi.sendShortMsg(channel,0x4A,0x26);
	}else{
	    midi.sendShortMsg(channel,0x4B,0x26);
	    midi.sendShortMsg(channel,0x4B,0x27);
	}
}

DNHS5500.bpm = function (channel, control, value, status) {
 	if (channel.charAt(8) == 1) {
        var channel = "0xB0";
    } else {
        var channel = "0xB1";
    }
	var BPMRound = Math.round(engine.getValue("[Channel1]","bpm"));
	print("BPM="+BPMRound);
	midi.sendShortMsg(channel, 0x73, BPMRound);
}

DNHS5500.Keylocklights = function (channel, control, value, status) {
    if (channel.charAt(8) == 1) {
        var channel = "0xB0";
    } else {
        var channel = "0xB1";
    }
    if (value !== 0) {
        midi.sendShortMsg(channel,0x4A,0x08);
    } else {
        midi.sendShortMsg(channel,0x4B,0x08);
    }
}

DNHS5500.rateDisplay = function (value, group) { // rate +/- display output
	// Note, this doesn't work for decks 3 and 4.
    if (group == "[Channel1]") {
        var channelmidi = "0xB0";
    } else {
        var channelmidi = "0xB1";
    }
	var raterange = engine.getValue(group, "rateRange");
	var rate = engine.getValue(group, "rate");

	//print("channelname"+channelname);
	//print("rateRange="+raterange);
	//print("pitch="+rate);
	//print("calculated pitch="+(rate*raterange)*100);
	var rate = ((rate * raterange) * 100);
	var ratemod = Math.abs(Math.round(rate * 100.0) / 100.0);
	//print("ratemod= "+ratemod);
	midi.sendShortMsg(channelmidi, 0x71, (Math.round(ratemod))); // Pitch MSB
	//String ratemod = String.valueOf(d);
	//ratemod = ratemod.subString ( ratemod.indexOf ( "." ) );
	var ratemod2 = (Math.round(ratemod));
	//print("ratemod2="+ratemod2);
	var ratemod3 = ((ratemod - ratemod2) * 100);
	//print("ratemod3="+ratemod3);
	//midi.sendShortMsg(channelmidi,0x72,ratemod);
	if (rate >= 0) {
    	midi.sendShortMsg(channelmidi, 0x45, 0x02);
	} else {
	    midi.sendShortMsg(channelmidi, 0x45, 0x01);
	}
}

DNHS5500.KeyLocker1 = function (channel, control, value, status) {
    if (channel == 0) {
        var channelmidi = "0xB0";
        var channelname = "[Channel1]";
    } else {
        var channelmidi = "0xB1";
        var channelname = "[Channel2]";
    }
	var currentKeyLock = engine.getValue(channelname,"keylock");
	if (currentKeyLock == 1) { // if currently keylocked`
	    engine.setValue(channelname, "keylock", 0); // Release Keylock
	    midi.sendShortMsg(channelmidi, 0x4B, 0x08);
	} else {
	    engine.setValue(channelname, "keylock", 1); // Set KeyLock
	    midi.sendShortMsg(channelmidi, 0x4A, 0x08);
	}
}

DNHS5500.PlayButton1 = function (channel, control, value, status) {    // Play button for deck 1
	print ("channel="+channel);
	if (channel == 0) {
	    var channelmidi = "0xB0";
	    var channelname = "[Channel1]";
	} else {
	    var channelmidi = "0xB1";
	    var channelname = "[Channel2]";
	}
	print ("channelmidi="+channelmidi);
	print ("channelname="+channelname);
   	var currentlyPlaying = engine.getValue(channelname,"play");
    if (currentlyPlaying == 1) {    // If currently playing
        engine.setValue(channelname,"play",0);    // Stop
     	var playpos = (engine.getValue(channelname,"playposition"));
	    var cuepoint = (engine.getValue(channelname,"cue_point"));
	    print ("cuepoint="+cuepoint);
        if (playpos !=cuepoint) {
            midi.sendShortMsg(channelmidi,0x4C,0x27);    // FIX BLINK Turn on the Play LED
            midi.sendShortMsg(channelmidi,0x4B,0x26);    // Turn off the CUE LED
	    } else {
            midi.sendShortMsg(channelmidi,0x4B,0x27);    // Turn off the Play LED
            midi.sendShortMsg(channelmidi,0x4A,0x26);    // Turn on the CUE LED
	    }
    } else {    // If not currently playing,
    	engine.setValue(channelname,"play",1);    // Start
        midi.sendShortMsg(channelmidi,0x4A,0x27);    // Turn on the Play LED
        midi.sendShortMsg(channelmidi,0x4B,0x26);    // Turn off the CUE LED
    }
}
