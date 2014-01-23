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

    DNHS5500.hasRecentWheelVelocity = [1, 1];
    DNHS5500.recentWheelVelocity = [0, 0];
    DNHS5500.scratchDecay = [0, 0];
}
DNHS5500.shutdown = function shutdown() {}

// The button that enables/disables scratching
DNHS5500.wheelTouch = function (channel, control, value, status) {
    // This is documented but never happens.
    // Jogwheel touch values.
    if (value == 0x40) {
        print ("wheel touch scratch on");
        DNHS5500.scratchEnable(channel + 1);
    }
}

DNHS5500.scratchEnable = function (deck) {
    print ("scratch on");
    var alpha = 1.0/64;
    var beta = alpha/64;
    engine.scratchEnable(deck, 1480, 33+1/3, alpha, beta);
}

DNHS5500.scratchEnableDamped = function (deck) {
    print ("scratch on damp");
    var alpha = 1.0/256;
    var beta = alpha/256;
    engine.scratchEnable(deck, 1480, 33+1/3, alpha, beta);
}

DNHS5500.wheelRelease = function (channel, control, value, status) {
    // This is documented but never happens.
    // Jogwheel touch values.
    if (value == 0x00) {
        engine.scratchDisable(channel + 1, true);
    }
}

DNHS5500.wheelTurn = function (channel, control, value, status) {
    // Reports the velocity of the top disc-like wheel, as well as touch events.
    var deckNumber = channel + 1;
    if (deckNumber == 1) {
        var group = "[Channel1]";
    } else if (deckNumber == 2) {
        var group = "[Channel2]";
    } else {
        print ("weird deck number " + channel);
        return;
    }

    // Velocity values.

    var velocity = value - 0x40;

    print ("wheel info!");
    DNHS5500.hasRecentWheelVelocity[channel] = true;
    DNHS5500.recentWheelVelocity[channel] = velocity;

    var currentlyPlaying = engine.getValue(group, "play");
    if (currentlyPlaying) {
        // If the velocity is not 1, we must be scratching.
        if (velocity != 1) {
            print ("playing, not 1 velo, scratch on");
            DNHS5500.scratchEnable(deckNumber);
        }
    }

    if (engine.isScratching(deckNumber)) {
        engine.scratchTick(deckNumber, velocity);
    } else {
        engine.setValue(group, "jog", velocity);
    }
}

//DNHS5500.resetScratchDecay = function (deckNumber) {
//    var deckIndex = deckNumber - 1;
//    print ("starting decay");
//    DNHS5500.scratchDecay[deckIndex] = 300;
//}

//DNHS5500.doScratchDecay = function (deckNumber) {
//    var deckIndex = deckNumber - 1;

//    print ("scratch decay " + DNHS5500.scratchDecay[deckIndex]);
//    DNHS5500.scratchDecay[deckIndex] -= 1;

//    if (DNHS5500.scratchDecay[deckIndex] <= 0) {
//        print ("decay complete, disabling scratch");
//        DNHS5500.scratchDecay[deckIndex] = 0;
//        engine.scratchDisable(deckNumber,);
//        print ("scratch disabled??? " + engine.isScratching(deckNumber));
//    }
//}

DNHS5500.platterTurn = function (channel, control, value, status) {
    // Reports the velocity of the lower platter.
    var velocity = value - 0x40;
    var deckNumber = channel + 1;
    if (deckNumber == 1) {
        var group = "[Channel1]";
    } else if (deckNumber == 2) {
        var group = "[Channel2]";
    } else {
        print ("weird deck number " + channel);
        return;
    }

    print ("platter turn");

    var currentlyPlaying = engine.getValue(group, "play");
    if (currentlyPlaying && velocity == 1) {
        if (!DNHS5500.hasRecentWheelVelocity[deckNumber - 1]) {
            if (!engine.isScratching(deckNumber)) {
                print ("no recent wheel info, think scratching");
                DNHS5500.scratchEnableDamped(deckNumber);
                //DNHS5500.resetScratchDecay(deckNumber);
            }
        } else if (DNHS5500.recentWheelVelocity[deckNumber - 1] == 1) {
            if (engine.isScratching(deckNumber)) {
                print ("think not scratching");
                //DNHS5500.scratchEnable(deckNumber, false);
                //DNHS5500.doScratchDecay(deckNumber);
                engine.scratchDisable(deckNumber, true);
                //DNHS5500.scratchDecay[deckNumber - 1] = 300;
            }
        }

        DNHS5500.hasRecentWheelVelocity[channel] = false;
    }
}

var duration = engine.getValue("[Channel1]","duration");
//print ("DURATION"+duration /60);
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
	    // Platter off
        midi.sendShortMsg(channelmidi,0x66,0x00);
        // Scratch on
        DNHS5500.scratchEnable(channel + 1);
        if (playpos !=cuepoint) {
            midi.sendShortMsg(channelmidi,0x4C,0x27);    // FIX BLINK Turn on the Play LED
            midi.sendShortMsg(channelmidi,0x4B,0x26);    // Turn off the CUE LED
	    } else {
            midi.sendShortMsg(channelmidi,0x4B,0x27);    // Turn off the Play LED
            midi.sendShortMsg(channelmidi,0x4A,0x26);    // Turn on the CUE LED
	    }
    } else {
        // Start playing.
    	engine.setValue(channelname, "play", 1);
        midi.sendShortMsg(channelmidi, 0x4A, 0x27);    // Turn on the Play LED
        midi.sendShortMsg(channelmidi, 0x4B, 0x26);    // Turn off the CUE LED
        midi.sendShortMsg(channelmidi, 0x66, 0x7F);    // Turn on platter!
        midi.sendShortMsg(channelmidi, 0x67, 0x00);    // platter FWD
        engine.scratchDisable(channel + 1, true);
    }
}
