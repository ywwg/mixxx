/****************************************************************/
/*      Vestax VCI-400 MIDI controller script v1.00             */
/*          Copyright (C) 2011, Tobias Rafreider                */
/*      but feel free to tweak this to your heart's content!    */
/*      For Mixxx version 1.11                                  */
/****************************************************************/


/*
Owen todo:
fx :(
vinyl button?
wheel touch?


add no-handler versions of play / pause / etc so we can put out the lights.
*/

/*
 * The VCI-400 class definition
 * All attributes of this class represent
 * buttons and controls affecting the MASTER output
 */
VestaxVCI400 = new function() {
   this.group = "[Master]";
   this.Controls = []; //The list of control objects, i.e., knobs
   this.Buttons = [];  //The list of buttons objects
}

VestaxVCI400.shiftActive = false;

VestaxVCI400.ModeEnum = {
    HOTCUE : 0,
    LOOP : 1,
    ROLL : 2,
    SAMPLER : 3,
}

VestaxVCI400.shiftActivate = function (channel, control, value, status, group) {
    VestaxVCI400.shiftActive = (value != 0);
}

/*
 * VU meters controls
 */
VestaxVCI400.onMasterVuMeterLChanged = function(value){
    var normalizedVal = parseInt(value*127);
    print("master vu " + normalizedVal);
    midi.sendShortMsg("0xbe", 43, normalizedVal);
}
VestaxVCI400.onMasterVuMeterRChanged = function(value){
    var normailizedVal = parseInt(value*127);
    midi.sendShortMsg("0xbe", 44, normailizedVal);

}

VestaxVCI400.ButtonLedState = {"on": 0x7F, "off": 0x00};
/*
 * Adds a button to the VCI-400 class object
 * The button must affect the MASTER.
 *
 * Parameters
 *  - buttonName: 'unique name of the button'
 *  - button 'the button object to be added
 *  - eventHandlder 'the class method which will be executed if button state has changed.
 */
VestaxVCI400.addButton = function(buttonName, button, eventHandler) {
   if(eventHandler) {
      var executionEnvironment = this;
      button.group = this.group;
      function handler(value) {
         button.state = value;
         executionEnvironment[eventHandler](value);
      }
      button.handler = handler;
   }
   this.Buttons[buttonName] = button; //Append the button the class attribute
};

/* ============BUTTON CLASS DEFINITION====================
 *
 * A button refers to push button control
 *
 * =======================================================
 */

VestaxVCI400.ButtonState = {"released":0x00, "pressed":0x7F};
/*
 * The button object
 */

VestaxVCI400.Button = function (statusByte, midiNo) {
   this.statusByte = statusByte;
   this.midiNo = midiNo;
   this.state = VestaxVCI400.ButtonState.released;
   this.illuminated = false;
   this.group; //this will be set when by the addButton method

};
/*
 * Button objects have a 'handleEvent' method
 * ,that is, the action to be performed if pressed.
 */
VestaxVCI400.Button.prototype.handleEvent = function(value) {
   this.handler(value);
};
//Calling this method will illuminate the button depening on if value is true or false
VestaxVCI400.Button.prototype.illuminate = function(value) {
   if(value ==true){
        midi.sendShortMsg(this.statusByte, this.midiNo, VestaxVCI400.ButtonLedState.on);
        this.illuminated = true;
    }
    else{
        midi.sendShortMsg(this.statusByte, this.midiNo, VestaxVCI400.ButtonLedState.off);
        this.illuminated = false;
    }
};

/*
 * =========DECK CLASS DEFINITION ===========================
 *
 * Although the VCI 400 has a 2 deck layout, you can
 * control up to 4 virtual decks.
 * Objects of this class do represent virtual decks.
 *
 *
 */



VestaxVCI400.Deck = function (deckNumber, group, active) {
   this.deckIdentifier = deckNumber;
   this.group = group;
   this.vinylMode = true;
   this.isActive = active; //if this deck is currently controlled by the VCI-400
   this.Buttons = []; //the buttons
   this.deckNumber = group.substring(8,9);// [Channel1]
   this.buttonMode = VestaxVCI400.ModeEnum.HOTCUE;
}
/*
 * Each deck has a disjunct set of buttons
 * This method adds a assigns a button to a deck
 */
VestaxVCI400.Deck.prototype.addButton = VestaxVCI400.addButton;


// ===========CREATING LEFT AND RIGHT DECKS OBJECTS
VestaxVCI400.Decks = {
    "A": new VestaxVCI400.Deck("A","[Channel1]", true), //Deck A and B will
    "B": new VestaxVCI400.Deck("B","[Channel2]", true),//be controlled by the VCI-400
    "C": new VestaxVCI400.Deck("C","[Channel3]", false),
    "D": new VestaxVCI400.Deck("D","[Channel4]", false)
};
VestaxVCI400.GroupToDeck = {
    "[Channel1]":"A",
    "[Channel2]":"B",
    "[Channel3]":"C",
    "[Channel4]":"D"
};
//returns the deck object given a group, e.g. "[Channel1]"
VestaxVCI400.GetDeck = function(group) {
   try {
      return VestaxVCI400.Decks[VestaxVCI400.GroupToDeck[group]];
   } catch(ex) {
      return null;
   }
}
// Vinyl button
VestaxVCI400.Deck.prototype.onVinyl = function(value) {
    this.vinylMode = (value == 127)? true: false;
};

//=========Pad Buttons=========================
//VestaxVCI400.Deck.prototype.onButton1Activate = function(value){
//    this.onDynamicButtonPressed(this.Buttons.BUTTON1, 1, value);
//};
//VestaxVCI400.Deck.prototype.onButton2Activate = function(value){
//    this.onDynamicButtonPressed(this.Buttons.BUTTON2, 2, value);
//};
//VestaxVCI400.Deck.prototype.onButton3Activate = function(value){
//    this.onDynamicButtonPressed(this.Buttons.BUTTON3, 3, value);
//};
//VestaxVCI400.Deck.prototype.onButton4Activate = function(value){
//    this.onDynamicButtonPressed(this.Buttons.BUTTON4, 4, value);
//};
VestaxVCI400.Deck.prototype.onButton5Activate = function(value){
    this.onDynamicButtonPressed(this.Buttons.BUTTON5, 1, value);
};
VestaxVCI400.Deck.prototype.onButton6Activate = function(value){
    this.onDynamicButtonPressed(this.Buttons.BUTTON6, 2, value);
};
VestaxVCI400.Deck.prototype.onButton7Activate = function(value){
    this.onDynamicButtonPressed(this.Buttons.BUTTON7, 3, value);
};
VestaxVCI400.Deck.prototype.onButton8Activate = function(value){
    this.onDynamicButtonPressed(this.Buttons.BUTTON8, 4, value);
};
VestaxVCI400.Deck.prototype.onDynamicButtonPressed = function(button, buttonNumber, value){
    switch(this.buttonMode) {
    case VestaxVCI400.ModeEnum.HOTCUE:
        var hotCueAction = "hotcue_".concat(buttonNumber.toString());
        if (VestaxVCI400.shiftActive) {
            hotCueAction = hotCueAction.concat("_clear");
        } else {
            hotCueAction = hotCueAction.concat("_activate");
        }
    	if(value == VestaxVCI400.ButtonState.pressed){
    		engine.setValue(this.group, hotCueAction, 1);
    	}
    	else{
    		engine.setValue(this.group, hotCueAction, 0);
    	}
    	break;
    case VestaxVCI400.ModeEnum.LOOP:
        var loop_size = Math.pow(2, buttonNumber);
        if(value == VestaxVCI400.ButtonState.pressed) {
            engine.setValue(this.group, "beatloop_" + loop_size.toString() + "_activate", 1);
        }
        break;
    case VestaxVCI400.ModeEnum.ROLL:
        var loop_size = Math.pow(2, buttonNumber - 4);
        if(value == VestaxVCI400.ButtonState.pressed) {
            engine.setValue(this.group, "beatlooproll_" + loop_size.toString() + "_activate", 1);
        } else {
            engine.setValue(this.group, "beatlooproll_" + loop_size.toString() + "_activate", 0);
        }
        break;
    case VestaxVCI400.ModeEnum.SAMPLER:
        if(value == VestaxVCI400.ButtonState.pressed) {
            engine.setValue("[Sampler" + buttonNumber + "]", "start_play", 1);
        } else {
            engine.setValue("[Sampler" + buttonNumber + "]", "stop", 1);
        }
        break;
    }
};

/*
 * The deck VU meters
 */
VestaxVCI400.Deck.prototype.onVuMeterChanged = function(value, group, key) {
     var normalizedVal = parseInt(value*127);
     var deckNumber = parseInt(group.substring(8,9));
     var midiNo = deckNumber + 1;
     var statusByte = "0xb".concat(midiNo.toString());
     midi.sendShortMsg(statusByte, 17, normalizedVal);
 };

 /*
  * This is called when Mixxx notifies us that a button state has changed
  * via engine.connect.
  */
VestaxVCI400.Deck.prototype.onHotCue1Changed = function(value, group, key) {
	try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.onHotCueChanged(deck.Buttons.BUTTON5, value);
	}
    catch(ex) {
    	VestaxVCI400.printError(ex);
    }
};
VestaxVCI400.Deck.prototype.onHotCue2Changed = function(value, group, key) {
	try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.onHotCueChanged(deck.Buttons.BUTTON6, value);
	}
    catch(ex) {
    	VestaxVCI400.printError(ex);
    }
};
VestaxVCI400.Deck.prototype.onHotCue3Changed = function(value, group, key) {
	try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.onHotCueChanged(deck.Buttons.BUTTON7, value);
	}
    catch(ex) {
    	VestaxVCI400.printError(ex);
    }
};
VestaxVCI400.Deck.prototype.onHotCue4Changed = function(value, group, key) {
	try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.onHotCueChanged(deck.Buttons.BUTTON8, value);
	}
    catch(ex) {
    	VestaxVCI400.printError(ex);
    }
};
//VestaxVCI400.Deck.prototype.onHotCue5Changed = function(value, group, key) {
//	try {
//        var deck = VestaxVCI400.GetDeck(group);
//        deck.onHotCueChanged(deck.Buttons.BUTTON5, value);
//	}
//    catch(ex) {
//    	VestaxVCI400.printError(ex);
//    }
//};
//VestaxVCI400.Deck.prototype.onHotCue6Changed = function(value, group, key) {
//	try {
//        var deck = VestaxVCI400.GetDeck(group);
//        deck.onHotCueChanged(deck.Buttons.BUTTON6, value);
//	}
//    catch(ex) {
//    	VestaxVCI400.printError(ex);
//    }
//};
//VestaxVCI400.Deck.prototype.onHotCue7Changed = function(value, group, key) {
//	try {
//        var deck = VestaxVCI400.GetDeck(group);
//        deck.onHotCueChanged(deck.Buttons.BUTTON7, value);
//	}
//    catch(ex) {
//    	VestaxVCI400.printError(ex);
//    }
//};
//VestaxVCI400.Deck.prototype.onHotCue8Changed = function(value, group, key) {
//	try {
//        var deck = VestaxVCI400.GetDeck(group);
//        deck.onHotCueChanged(deck.Buttons.BUTTON4, value);
//	}
//    catch(ex) {
//    	VestaxVCI400.printError(ex);
//    }
//};
/*
 * This is called when Mixxx notifies us that a button state has changed
 * via engine.connect.
 */

VestaxVCI400.Deck.prototype.onHotCueChanged = function(button, value) {
    try {
        if (this.buttonMode == VestaxVCI400.ModeEnum.HOTCUE) {
            if(value == 0) {
                button.illuminate(false); //turn off LEDs
            }
            else {
                button.illuminate(true); //turn LED on
            }
        }
    }
    catch(ex) {
        VestaxVCI400.printError(ex);
    }
};

  /*
   * This is called when Mixxx notifies us that a button state has changed
   * via engine.connect.
   */
VestaxVCI400.Deck.prototype.onLoopStatusChanged = function(value, group, key) {
		try {
	        var deck = VestaxVCI400.GetDeck(group);
	        //if loop has been enabled, illuminate LEDs
	        if(value == 1){
	        	deck.Buttons.LOOP_IN.illuminate(true);
	        	deck.Buttons.LOOP_OUT.illuminate(true);
	        }
	        else{
	        	deck.Buttons.LOOP_IN.illuminate(false);
	        	deck.Buttons.LOOP_OUT.illuminate(false);
	        }
		}
	    catch(ex) {
	    	VestaxVCI400.printError(ex);
	    }
	};
/*
 * Method to set the initial state of a deck
 */
VestaxVCI400.Deck.prototype.init = function() {
   // Make sure all buttons are not illuminated
   for(b in this.Buttons){
        this.Buttons[b].illuminate(false);
    }
    /*
     * Make sure vinyl LED is on as it is the default when switching on the VCI-400
     * Unfortunately, the VINYL buton has no LED receive slot, so we can't
     * enforce the button to be illuminated from a programming point of view.
     * This, however, is not a real limitation. The button is illuminated
     * after the device has been booted.
     */
     this.Buttons.VINYL.illuminate(true);

     //Connect controls
     engine.connectControl(this.group,"VuMeter", "VestaxVCI400.Decks."+this.deckIdentifier+".onVuMeterChanged");
     this.onVuMeterChanged(0, this.group, 0);

     engine.connectControl(this.group,"hotcue_1_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue1Changed");
     engine.connectControl(this.group,"hotcue_2_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue2Changed");
     engine.connectControl(this.group,"hotcue_3_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue3Changed");
     engine.connectControl(this.group,"hotcue_4_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue4Changed");
     engine.connectControl(this.group,"hotcue_5_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue5Changed");
     engine.connectControl(this.group,"hotcue_6_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue6Changed");
     engine.connectControl(this.group,"hotcue_7_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue7Changed");
     engine.connectControl(this.group,"hotcue_8_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue8Changed");

//     engine.connectControl("[Sampler1]","duration", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue1Changed");
//     engine.connectControl(this.group,"hotcue_2_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue2Changed");
//     engine.connectControl(this.group,"hotcue_3_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue3Changed");
//     engine.connectControl(this.group,"hotcue_4_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue4Changed");
//     engine.connectControl(this.group,"hotcue_5_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue5Changed");
//     engine.connectControl(this.group,"hotcue_6_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue6Changed");
//     engine.connectControl(this.group,"hotcue_7_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue7Changed");
//     engine.connectControl(this.group,"hotcue_8_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue8Changed");

     engine.connectControl(this.group,"hotcue_1_enabled", "VestaxVCI400.Decks."+this.deckIdentifier+".onHotCue1Changed");

};
//Creating buttons referring to left deck

VestaxVCI400.Decks.A.addButton("VINYL", new VestaxVCI400.Button(0xB2,0x06), "onVinyl");
VestaxVCI400.Decks.B.addButton("VINYL", new VestaxVCI400.Button(0xB3,0x06), "onVinyl");
VestaxVCI400.Decks.C.addButton("VINYL", new VestaxVCI400.Button(0xB4,0x06), "onVinyl");
VestaxVCI400.Decks.D.addButton("VINYL", new VestaxVCI400.Button(0xB5,0x06), "onVinyl");

VestaxVCI400.Decks.A.addButton("WHEEL", new VestaxVCI400.Button(0xB2,0x27), "onWheelTouch");
VestaxVCI400.Decks.B.addButton("WHEEL", new VestaxVCI400.Button(0xB3,0x27), "onWheelTouch");
VestaxVCI400.Decks.C.addButton("WHEEL", new VestaxVCI400.Button(0xB4,0x27), "onWheelTouch");
VestaxVCI400.Decks.D.addButton("WHEEL", new VestaxVCI400.Button(0xB5,0x27), "onWheelTouch");

VestaxVCI400.Decks.A.addButton("MODE_HOTCUE", new VestaxVCI400.Button(0x92,0x15), "onModeHotcue");
VestaxVCI400.Decks.B.addButton("MODE_HOTCUE", new VestaxVCI400.Button(0x93,0x15), "onModeHotcue");
VestaxVCI400.Decks.C.addButton("MODE_HOTCUE", new VestaxVCI400.Button(0x94,0x15), "onModeHotcue");
VestaxVCI400.Decks.D.addButton("MODE_HOTCUE", new VestaxVCI400.Button(0x95,0x15), "onModeHotcue");

VestaxVCI400.Decks.A.addButton("MODE_LOOP", new VestaxVCI400.Button(0x92,0x16), "onModeLoop");
VestaxVCI400.Decks.B.addButton("MODE_LOOP", new VestaxVCI400.Button(0x93,0x16), "onModeLoop");
VestaxVCI400.Decks.C.addButton("MODE_LOOP", new VestaxVCI400.Button(0x94,0x16), "onModeLoop");
VestaxVCI400.Decks.D.addButton("MODE_LOOP", new VestaxVCI400.Button(0x95,0x16), "onModeLoop");

VestaxVCI400.Decks.A.addButton("MODE_ROLL", new VestaxVCI400.Button(0x92,0x17), "onModeRoll");
VestaxVCI400.Decks.B.addButton("MODE_ROLL", new VestaxVCI400.Button(0x93,0x17), "onModeRoll");
VestaxVCI400.Decks.C.addButton("MODE_ROLL", new VestaxVCI400.Button(0x94,0x17), "onModeRoll");
VestaxVCI400.Decks.D.addButton("MODE_ROLL", new VestaxVCI400.Button(0x95,0x17), "onModeRoll");

VestaxVCI400.Decks.A.addButton("MODE_SAMPLER", new VestaxVCI400.Button(0x92,0x18), "onModeSampler");
VestaxVCI400.Decks.B.addButton("MODE_SAMPLER", new VestaxVCI400.Button(0x93,0x18), "onModeSampler");
VestaxVCI400.Decks.C.addButton("MODE_SAMPLER", new VestaxVCI400.Button(0x94,0x18), "onModeSampler");
VestaxVCI400.Decks.D.addButton("MODE_SAMPLER", new VestaxVCI400.Button(0x95,0x18), "onModeSampler");


VestaxVCI400.Decks.A.addButton("BUTTON1", new VestaxVCI400.Button(0x92,0x07), "onButton1Activate");
VestaxVCI400.Decks.A.addButton("BUTTON2", new VestaxVCI400.Button(0x92,0x08), "onButton2Activate");
VestaxVCI400.Decks.A.addButton("BUTTON3", new VestaxVCI400.Button(0x92,0x09), "onButton3Activate");
VestaxVCI400.Decks.A.addButton("BUTTON4", new VestaxVCI400.Button(0x92,0x0A), "onButton4Activate");
VestaxVCI400.Decks.A.addButton("BUTTON5", new VestaxVCI400.Button(0x92,0x0B), "onButton5Activate");
VestaxVCI400.Decks.A.addButton("BUTTON6", new VestaxVCI400.Button(0x92,0x0C), "onButton6Activate");
VestaxVCI400.Decks.A.addButton("BUTTON7", new VestaxVCI400.Button(0x92,0x0D), "onButton7Activate");
VestaxVCI400.Decks.A.addButton("BUTTON8", new VestaxVCI400.Button(0x92,0x0E), "onButton8Activate");

VestaxVCI400.Decks.B.addButton("BUTTON1", new VestaxVCI400.Button(0x93,0x07), "onButton1Activate");
VestaxVCI400.Decks.B.addButton("BUTTON2", new VestaxVCI400.Button(0x93,0x08), "onButton2Activate");
VestaxVCI400.Decks.B.addButton("BUTTON3", new VestaxVCI400.Button(0x93,0x09), "onButton3Activate");
VestaxVCI400.Decks.B.addButton("BUTTON4", new VestaxVCI400.Button(0x93,0x0A), "onButton4Activate");
VestaxVCI400.Decks.B.addButton("BUTTON5", new VestaxVCI400.Button(0x93,0x0B), "onButton5Activate");
VestaxVCI400.Decks.B.addButton("BUTTON6", new VestaxVCI400.Button(0x93,0x0C), "onButton6Activate");
VestaxVCI400.Decks.B.addButton("BUTTON7", new VestaxVCI400.Button(0x93,0x0D), "onButton7Activate");
VestaxVCI400.Decks.B.addButton("BUTTON8", new VestaxVCI400.Button(0x93,0x0E), "onButton8Activate");

VestaxVCI400.Decks.C.addButton("BUTTON1", new VestaxVCI400.Button(0x94,0x07), "onButton1Activate");
VestaxVCI400.Decks.C.addButton("BUTTON2", new VestaxVCI400.Button(0x94,0x08), "onButton2Activate");
VestaxVCI400.Decks.C.addButton("BUTTON3", new VestaxVCI400.Button(0x94,0x09), "onButton3Activate");
VestaxVCI400.Decks.C.addButton("BUTTON4", new VestaxVCI400.Button(0x94,0x0A), "onButton4Activate");
VestaxVCI400.Decks.C.addButton("BUTTON5", new VestaxVCI400.Button(0x94,0x0B), "onButton5Activate");
VestaxVCI400.Decks.C.addButton("BUTTON6", new VestaxVCI400.Button(0x94,0x0C), "onButton6Activate");
VestaxVCI400.Decks.C.addButton("BUTTON7", new VestaxVCI400.Button(0x94,0x0D), "onButton7Activate");
VestaxVCI400.Decks.C.addButton("BUTTON8", new VestaxVCI400.Button(0x94,0x0E), "onButton8Activate");

VestaxVCI400.Decks.D.addButton("BUTTON1", new VestaxVCI400.Button(0x95,0x07), "onButton1Activate");
VestaxVCI400.Decks.D.addButton("BUTTON2", new VestaxVCI400.Button(0x95,0x08), "onButton2Activate");
VestaxVCI400.Decks.D.addButton("BUTTON3", new VestaxVCI400.Button(0x95,0x09), "onButton3Activate");
VestaxVCI400.Decks.D.addButton("BUTTON4", new VestaxVCI400.Button(0x95,0x0A), "onButton4Activate");
VestaxVCI400.Decks.D.addButton("BUTTON5", new VestaxVCI400.Button(0x95,0x0B), "onButton5Activate");
VestaxVCI400.Decks.D.addButton("BUTTON6", new VestaxVCI400.Button(0x95,0x0C), "onButton6Activate");
VestaxVCI400.Decks.D.addButton("BUTTON7", new VestaxVCI400.Button(0x95,0x0D), "onButton7Activate");
VestaxVCI400.Decks.D.addButton("BUTTON8", new VestaxVCI400.Button(0x95,0x0E), "onButton8Activate");


/* ================MAPPING FUNCTIONS ==========================
 *
 * Having defined the some objects and class definitions
 * we can now easily build the mapping
 */


/*
 * Called when the MIDI device is opened for set up
 */
VestaxVCI400.init = function (id) {
   engine.setValue("[Master]", "num_decks", 4);
   //Initialize controls and their default values here
   VestaxVCI400.Decks.A.init();
   VestaxVCI400.Decks.B.init();
   VestaxVCI400.Decks.C.init();
   VestaxVCI400.Decks.D.init();

   //Connect vu meters
   engine.connectControl("[Master]","VuMeterL", "VestaxVCI400.onMasterVuMeterLChanged");
   engine.connectControl("[Master]","VuMeterR", "VestaxVCI400.onMasterVuMeterRChanged");

   //Reset VU meters
   midi.sendShortMsg("0xbe", 43, 0);
   midi.sendShortMsg("0xbe", 44, 0);
};

/*
 * Called when the MIDI device is closed
 */
VestaxVCI400.shutdown = function () {
	//Initialize controls and their default values here
	VestaxVCI400.Decks.A.init();
	VestaxVCI400.Decks.B.init();
	VestaxVCI400.Decks.C.init();
	VestaxVCI400.Decks.D.init();

	//Reset VU meters
	midi.sendShortMsg("0xbe", 43, 0);
	midi.sendShortMsg("0xbe", 44, 0);
};

VestaxVCI400.vinyl = function (channel, control, value, status, group) {
    try{
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.VINYL.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};
/*
 * Mapping deck selection switch
 */
VestaxVCI400.deckSwitch = function (channel, control, value, status, group) {
    var deck = VestaxVCI400.GetDeck(group);
    deck.isActive = (value == 127)? true: false;
    print("Deck "+deck.deckIdentifier+ " is controlled by VCI-400: "+deck.isActive);
};
/*
 * Mapping wheel touch
 */
VestaxVCI400.wheelTouch = function (channel, control, value, status, group) {
    try{
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.WHEEL.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};
VestaxVCI400.Deck.prototype.onWheelTouch = function(value) {
//    if(this.vinylMode == false)
//        return;

    if(value == VestaxVCI400.ButtonState.pressed) {
        engine.scratchEnable(this.deckNumber, 4096, 33.3333, 0.125, 0.125/32, true);
    } else {
        // ramp goes directly to zero so we don't want that.
        engine.scratchDisable(this.deckNumber, false);
    }
};

/*
 * Mapping wheel motion
 */
VestaxVCI400.wheelMove = function (channel, control, value, status, group) {
    try{
        var deck = VestaxVCI400.GetDeck(group);
        deck.onWheelMove(value);

    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};
VestaxVCI400.Deck.prototype.onWheelMove = function(value) {
    var jogValue = value - 0x40; // -64 to +63, - = CCW, + = CW
    engine.setValue(this.group, "jog", jogValue / 10);
    if(engine.getValue(this.group, "scratch2_enable")){
        engine.scratchTick(this.deckNumber, jogValue);
    }
};

/*
 * Pad Buttons
 */
VestaxVCI400.button1Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON1.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button2Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON2.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button3Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON3.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button4Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON4.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button5Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON5.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button6Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON6.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button7Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON7.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};
VestaxVCI400.button8Activate = function (channel, control, value, status, group) {
    try{
       var deck = VestaxVCI400.GetDeck(group);
       deck.Buttons.BUTTON8.handleEvent(value);
   }
   catch(ex) {
       VestaxVCI400.printError(ex);
  }
};

VestaxVCI400.loopKnob = function (channel, control, value, status, group) {
	try{
        var deck = VestaxVCI400.GetDeck(group);
        var isLoopActive = engine.getValue(deck.group, "loop_enabled");
        var jogValue = value - 0x40; // -64 to +63, - = CCW, + = CW

        if(isLoopActive){
            if (VestaxVCI400.shiftActive) {
            	if(jogValue > 0)
            		engine.setValue(deck.group, "loop_move", -1)
            	else
            		engine.setValue(deck.group, "loop_move", 1)
            } else {
            	if(jogValue > 0) {
            		engine.setValue(deck.group, "loop_halve", 1)
            		engine.setValue(deck.group, "loop_halve", 0)
            	} else {
            		engine.setValue(deck.group, "loop_double", 1)
            		engine.setValue(deck.group, "loop_double", 0)
                }
            }
        }
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
}

VestaxVCI400.printError = function(exception){
	print("Error Msg: "+exception.toString());
};


/*** Owen's shit ***/

VestaxVCI400.modeHotcue = function (channel, control, value, status, group) {
    try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.MODE_HOTCUE.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};

VestaxVCI400.modeLoop = function (channel, control, value, status, group) {
    try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.MODE_LOOP.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};

VestaxVCI400.modeRoll = function (channel, control, value, status, group) {
    try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.MODE_ROLL.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};

VestaxVCI400.modeSampler = function (channel, control, value, status, group) {
    try {
        var deck = VestaxVCI400.GetDeck(group);
        deck.Buttons.MODE_SAMPLER.handleEvent(value);
    }
    catch(ex) {
    	VestaxVCI400.printError(ex);
   }
};

VestaxVCI400.Deck.prototype.lightButton = function(index, on, dim) {

}

VestaxVCI400.Deck.prototype.onModeHotcue = function(value) {
    if(value == VestaxVCI400.ButtonState.pressed) {
        this.buttonMode = VestaxVCI400.ModeEnum.HOTCUE;
        this.Buttons.MODE_HOTCUE.illuminate(true);

        this.onHotCueChanged(this.Buttons.BUTTON5, engine.getValue(this.group, "hotcue_1_enabled"));
        this.onHotCueChanged(this.Buttons.BUTTON6, engine.getValue(this.group, "hotcue_2_enabled"));
        this.onHotCueChanged(this.Buttons.BUTTON7, engine.getValue(this.group, "hotcue_3_enabled"));
        this.onHotCueChanged(this.Buttons.BUTTON8, engine.getValue(this.group, "hotcue_4_enabled"));
//        this.onHotCueChanged(this.Buttons.BUTTON5, engine.getValue(this.group, "hotcue_5_enabled"));
//        this.onHotCueChanged(this.Buttons.BUTTON6, engine.getValue(this.group, "hotcue_6_enabled"));
//        this.onHotCueChanged(this.Buttons.BUTTON7, engine.getValue(this.group, "hotcue_7_enabled"));
//        this.onHotCueChanged(this.Buttons.BUTTON8, engine.getValue(this.group, "hotcue_8_enabled"));
    } else {
        this.Buttons.MODE_HOTCUE.illuminate(false);
    }
}

VestaxVCI400.Deck.prototype.onModeLoop = function(value) {
    if(value == VestaxVCI400.ButtonState.pressed) {
        this.buttonMode = VestaxVCI400.ModeEnum.LOOP;
        this.Buttons.MODE_LOOP.illuminate(true);

        this.Buttons.BUTTON5.illuminate(true);
        this.Buttons.BUTTON6.illuminate(true);
        this.Buttons.BUTTON7.illuminate(true);
        this.Buttons.BUTTON8.illuminate(true);
//        this.Buttons.BUTTON5.illuminate(true);
//        this.Buttons.BUTTON6.illuminate(true);
//        this.Buttons.BUTTON7.illuminate(true);
//        this.Buttons.BUTTON8.illuminate(true);
    } else {
        this.Buttons.MODE_LOOP.illuminate(false);
    }
}

VestaxVCI400.Deck.prototype.onModeRoll = function(value) {
    if(value == VestaxVCI400.ButtonState.pressed) {
        this.buttonMode = VestaxVCI400.ModeEnum.ROLL;
        this.Buttons.MODE_ROLL.illuminate(true);
        this.Buttons.BUTTON5.illuminate(true);
        this.Buttons.BUTTON6.illuminate(true);
        this.Buttons.BUTTON7.illuminate(true);
        this.Buttons.BUTTON8.illuminate(true);
//        this.Buttons.BUTTON5.illuminate(true);
//        this.Buttons.BUTTON6.illuminate(true);
//        this.Buttons.BUTTON7.illuminate(true);
//        this.Buttons.BUTTON8.illuminate(true);
    } else {
        this.Buttons.MODE_ROLL.illuminate(false);
    }
}

VestaxVCI400.Deck.prototype.onModeSampler = function(value) {
    if(value == VestaxVCI400.ButtonState.pressed) {
        this.buttonMode = VestaxVCI400.ModeEnum.SAMPLER;
        this.Buttons.MODE_SAMPLER.illuminate(true);
        this.Buttons.BUTTON5.illuminate(false);
        this.Buttons.BUTTON6.illuminate(false);
        this.Buttons.BUTTON7.illuminate(false);
        this.Buttons.BUTTON8.illuminate(false);
//        this.Buttons.BUTTON5.illuminate(false);
//        this.Buttons.BUTTON6.illuminate(false);
//        this.Buttons.BUTTON7.illuminate(false);
//        this.Buttons.BUTTON8.illuminate(false);
   } else {
        this.Buttons.MODE_SAMPLER.illuminate(false);
    }
}
