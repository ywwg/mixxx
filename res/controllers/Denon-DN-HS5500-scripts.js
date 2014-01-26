// (called <manufacturer><device>.init(ID,debugging) and <manufacturer><device>.shutdown() )
function DNHS5500() {}
DNHS5500.debug=3;

DNHS5500.init = function init(id,debug) {
    engine.connectControl("[Channel1]","track_samples","DNHS5500.loadLights");
    engine.connectControl("[Channel2]","track_samples","DNHS5500.loadLights");
    engine.connectControl("[Channel1]","rate", "DNHS5500.rateDisplay");
    engine.connectControl("[Channel2]","rate", "DNHS5500.rateDisplay");
    engine.connectControl("[Channel1]", "play", "DNHS5500.playChanged");
    engine.connectControl("[Channel2]", "play", "DNHS5500.playChanged");
    engine.connectControl("[Channel1]", "spinny_angle", "DNHS5500.spinnyAngleChanged");
    engine.connectControl("[Channel2]", "spinny_angle", "DNHS5500.spinnyAngleChanged");
    engine.connectControl("[Channel1]", "playposition", "DNHS5500.playPositionChanged");
    engine.connectControl("[Channel2]", "playposition", "DNHS5500.playPositionChanged");
    engine.connectControl("[Channel1]","eject","DNHS5500.eject");
    engine.connectControl("[Channel2]","eject","DNHS5500.eject");


    // Lights on this controller are chosen by midi value, not control value.
    engine.connectControl("[Channel1]","keylock","DNHS5500.keylockLight");
    engine.connectControl("[Channel2]","keylock","DNHS5500.keylockLight");
    engine.connectControl("[Channel1]","hotcue_1_enabled","DNHS5500.hotcue1Light");
    engine.connectControl("[Channel2]","hotcue_1_enabled","DNHS5500.hotcue1Light");
    engine.connectControl("[Channel1]","hotcue_2_enabled","DNHS5500.hotcue2Light");
    engine.connectControl("[Channel2]","hotcue_2_enabled","DNHS5500.hotcue2Light");
    engine.connectControl("[Channel1]","hotcue_3_enabled","DNHS5500.hotcue3Light");
    engine.connectControl("[Channel2]","hotcue_3_enabled","DNHS5500.hotcue3Light");
    engine.connectControl("[Channel1]","reverse","DNHS5500.reverseLight");
    engine.connectControl("[Channel2]","reverse","DNHS5500.reverseLight");
    engine.connectControl("[Channel1]","scratch2_enable","DNHS5500.scratchLight");
    engine.connectControl("[Channel2]","scratch2_enable","DNHS5500.scratchLight");
    engine.connectControl("[Channel1]","loop_enabled","DNHS5500.loopEnabledLight");
    engine.connectControl("[Channel2]","loop_enabled","DNHS5500.loopEnabledLight");
//    engine.connectControl("[Channel1]","reloop_exit","DNHS5500.exitReloopLight");
//    engine.connectControl("[Channel2]","reloop_exit","DNHS5500.exitReloopLight");
    engine.connectControl("[Channel1]","loop_start_position","DNHS5500.loopInLight");
    engine.connectControl("[Channel2]","loop_start_position","DNHS5500.loopInLight");
    engine.connectControl("[Channel1]","loop_end_position","DNHS5500.loopOutLight");
    engine.connectControl("[Channel2]","loop_end_position","DNHS5500.loopOutLight");
    engine.connectControl("[Channel1]","repeat","DNHS5500.repeatLight");
    engine.connectControl("[Channel2]","repeat","DNHS5500.repeatLight");

    DNHS5500.repeatLight(0, "[Channel1]");
    DNHS5500.repeatLight(0, "[Channel2]");

    // The jog wheel does not send events when it's not moving, so we have to
    // detect that.  We do so by adding one to this accumulator every time
    // we get a platter rotation message, and resetting it to zero every time
    // we get a real jog wheel message.  When the accumulator gets above a magic
    // number, we assume that the platter is stopped and send a zero.
    DNHS5500.wheelZeroVelocityAccum = [0, 0];

    DNHS5500.scratchEnable(1);
    DNHS5500.scratchEnable(2);
}

DNHS5500.shutdown = function shutdown() {
    // Turn off platter movement.
    midi.sendShortMsg(0xB0, 0x66, 0x00);
    midi.sendShortMsg(0xB1, 0x66, 0x00);
}

DNHS5500.channelForGroup = function (group) {
    if (group == "[Channel1]" || group == "[Channel3]") {
        return 0xB0;
    } else {
        return 0xB1;
    }
}

// XXX: how to support decks 3 and 4?
DNHS5500.groupForChannel = function (channel) {
    if (channel == 0) {
        return "[Channel1]";
    } else {
        return "[Channel2]";
    }
}

DNHS5500.rateDisplay = function (value, group) { // rate +/- display output
	// Note, this doesn't work for decks 3 and 4.
	var channelmidi = DNHS5500.channelForGroup(group);
    var rateDir = engine.getValue(group, "rate_dir");
	var raterange = engine.getValue(group, "rateRange");
	var rate = engine.getValue(group, "rate");
	var slider_rate = ((rate * raterange) * 100) * rateDir;
	var rate_abs = Math.abs(slider_rate);
	var rate_dec = Math.floor(rate_abs);
	var rate_frac = Math.round((rate_abs - rate_dec) * 100);

	midi.sendShortMsg(channelmidi, 0x71, rate_dec);
	midi.sendShortMsg(channelmidi, 0x72, rate_frac);

	// +/-
	if (rate >= 0) {
    	midi.sendShortMsg(channelmidi, 0x45, 0x01);
	} else {
	    midi.sendShortMsg(channelmidi, 0x45, 0x02);
	}

    // BPM
    var bpm = engine.getValue(group, "bpm");
    // bpm display is split like MML.L
    var bpm_dec = Math.floor(bpm / 10);
    var bpm_frac = Math.round((bpm - (bpm_dec * 10)) * 10);
    midi.sendShortMsg(channelmidi, 0x73, bpm_dec);
    midi.sendShortMsg(channelmidi, 0x74, bpm_frac);
}

DNHS5500.rateDisplayClear = function (group) {
    midi.sendShortMsg(channelmidi, 0x71, 0);
	midi.sendShortMsg(channelmidi, 0x72, 0);
 	midi.sendShortMsg(channelmidi, 0x45, 0x01);
    midi.sendShortMsg(channelmidi, 0x73, 0);
    midi.sendShortMsg(channelmidi, 0x74, 0);
}

DNHS5500.playPositionChanged = function (value, group) {
	var channelmidi = DNHS5500.channelForGroup(group);

    // Track percentage position.
	var reversed = engine.getValue(group, "reverse");
	if (reversed) {
	    midi.sendShortMsg(channelmidi, 0x49, Math.round(value * 100));
	} else {
    	midi.sendShortMsg(channelmidi, 0x48, Math.round(value * 100));
    }

    // MM:SS.FF
    var duration = engine.getValue(group, "duration");
    var pos_total_secs = value * duration;
    var pos_minutes = Math.floor(pos_total_secs / 60);
    var pos_secs = pos_total_secs % 60;
    var pos_frac = Math.round((pos_total_secs - Math.floor(pos_total_secs)) * 100);

    midi.sendShortMsg(channelmidi, 0x42, pos_minutes);
    midi.sendShortMsg(channelmidi, 0x43, pos_secs);
    midi.sendShortMsg(channelmidi, 0x44, pos_frac);

}

DNHS5500.spinnyAngleChanged = function (angle, group) {
	var channelmidi = DNHS5500.channelForGroup(group);
    // The angle is 0-180 to the right, and 0- -180 to the left.
    var midi_value = 0;
    if (angle >= 0) {
        // Values above and equal zero are mapped from 0x22 to 0x31, which is a range of 16 (0x10).
        midi_value = Math.round(angle * 16.0 / 180.0 + 0x22);
    } else {
        // Values below zero are mapped from 0x41 to 0x32, also 16 (0x10).
        // But the midi numbers are flopped versus the angle values.
        midi_value = Math.round((180.0 + angle) * 16.0 / 180.0 + 0x31);
    }
    midi.sendShortMsg(channelmidi, 0x4D, midi_value);
}

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

//DNHS5500.scratchEnableDamped = function (deck) {
//    print ("scratch on damp");
//    var alpha = 1.0/256;
//    var beta = alpha/256;
//    engine.scratchEnable(deck, 1480, 33+1/3, alpha, beta);
//}

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
    var group = DNHS5500.groupForChannel(channel);

    // Velocity values.
    var velocity = value - 0x40;
    // We use control == 0 when making calls internally.
    if (control != 0) {
        // We got a real update, so reset the accumulator.
        DNHS5500.wheelZeroVelocityAccum[channel] = 0;
    }

    var currentlyPlaying = engine.getValue(group, "play");
    if (currentlyPlaying) {
        // If we are playing, a velocity of 1 means no change.
        velocity -= 1;
    }

    if (engine.isScratching(deckNumber)) {
        engine.scratchTick(deckNumber, velocity);
    } else {
        engine.setValue(group, "jog", velocity);
    }
}

DNHS5500.platterTurn = function (channel, control, value, status) {
    // Reports the velocity of the lower platter.
    var velocity = value - 0x40;
    var deckNumber = channel + 1;
    var group = DNHS5500.groupForChannel(channel);

    var currentlyPlaying = engine.getValue(group, "play");
    if (currentlyPlaying && velocity == 1) {
        DNHS5500.wheelZeroVelocityAccum[channel] += 1;
        // If the accumulator doesn't get reset, the wheel is stopped.
        if (DNHS5500.wheelZeroVelocityAccum[channel] > 3) {
            // Fake a zero
            DNHS5500.wheelTurn(channel, 0, 0x40, 0);
        }
    }
}

DNHS5500.playButton = function (channel, control, value, status) {
    // Only respond to presses.
	if (value == 0) {
	    return;
	}
	var channelname = DNHS5500.groupForChannel(channel);

	var currentlyPlaying = engine.getValue(channelname,"play");
	// Toggle it.
	if (currentlyPlaying) {
    	engine.setValue(channelname, "play", 0.0);
	} else {
    	engine.setValue(channelname, "play", 1.0);
	}
	DNHS5500.playChanged(channel, control, value, status);
}

DNHS5500.playChanged = function (value, group) {
	var channelmidi = DNHS5500.channelForGroup(group);
	if (group == "[Channel1]") {
	    var deck = 1;
	} else {
	    var deck = 2;
	}

   	var currentlyPlaying = engine.getValue(group, "play");
    if (currentlyPlaying == 1) {
        // Turn on the Play LED
        midi.sendShortMsg(channelmidi, 0x4A, 0x27);
        // Turn off the CUE LED
        midi.sendShortMsg(channelmidi, 0x4B, 0x26);
        // Turn on platter!
        midi.sendShortMsg(channelmidi, 0x66, 0x7F);
        // platter FWD
        midi.sendShortMsg(channelmidi, 0x67, 0x00);
        // Disable scratch mode.
        engine.scratchDisable(deck, false);
    } else {
	    // Platter off
        midi.sendShortMsg(channelmidi, 0x66, 0x00);
        // Scratch on
        DNHS5500.scratchEnable(deck);
    }
}

DNHS5500.eject = function (channel, control, value, status) {
    if (value == 0) {
        return;
    }

    var group = DNHS5500.groupForChannel(channel);

    DNHS5500.hotcue1Light(0, group);
    DNHS5500.hotcue2Light(0, group);
    DNHS5500.hotcue3Light(0, group);
    DNHS5500.loopInLight(0, group);
    DNHS5500.loopOutLight(0, group);
    DNHS5500.rateDispalyClear(group);
    DNHS5500.spinnyAngleChanged(0, group);
    DNHS5500.playPositionChanged(0, group);
}

DNHS5500.loadLights = function (value, group) {
	var channel = DNHS5500.channelForGroup(group);
}

DNHS5500.toggleLightLayer1 = function (group, light, status) {
	var channel = DNHS5500.channelForGroup(group);
    if (status > 0) {
        midi.sendShortMsg(channel, 0x4A, light);
    } else {
        midi.sendShortMsg(channel, 0x4B, light);
    }
}

DNHS5500.toggleLightLayer2 = function (group, light, status) {
	var channel = DNHS5500.channelForGroup(group);
    if (status > 0) {
        midi.sendShortMsg(channel, 0x4D, light);
    } else {
        midi.sendShortMsg(channel, 0x4E, light);
    }
}

DNHS5500.keylockLight = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x08, value);
    DNHS5500.toggleLightLayer2(group, 0x14, value);
}

DNHS5500.hotcue1Light = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x11, value);
}

DNHS5500.hotcue2Light = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x13, value);
}

DNHS5500.hotcue3Light = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x15, value);
}

DNHS5500.reverseLight = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x3A, value);
}

DNHS5500.scratchLight = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x3B, value);
}

DNHS5500.exitReloopLight = function (value, group) {
    DNHS5500.toggleLightLayer1(group, 0x42, value);
}

DNHS5500.loopEnabledLight = function (value, group) {
    if (value != 0) {
        // Include the ()
        DNHS5500.toggleLightLayer2(group, 0x42, 1);
        DNHS5500.toggleLightLayer2(group, 0x44, 1);
    } else {
        // Turn off ()
        DNHS5500.toggleLightLayer2(group, 0x1A, 1);
        DNHS5500.toggleLightLayer2(group, 0x1C, 1);
    }
}

DNHS5500.loopInLight = function (value, group) {
    var enabled = (value != -1);
    DNHS5500.toggleLightLayer2(group, 0x1A, enabled);
}

DNHS5500.loopOutLight = function (value, group) {
    var enabled = (value != -1);
    DNHS5500.toggleLightLayer2(group, 0x1C, enabled);
}

DNHS5500.repeatLight = function (value, group) {
    if (value > 0) {
        DNHS5500.toggleLightLayer2(group, 0x04, 1);
    } else {
        DNHS5500.toggleLightLayer2(group, 0x05, 1);
    }
}
