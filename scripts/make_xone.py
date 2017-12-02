#!/usr/bin/python

#############################################################################
#                          make_xone.py  - Ugly script which automatically
#                          creates a midi mapping for the Xone K2.
#                          Lots of this is boilerplate...
#                             -------------------
#    begin                : Fri Aug 31 18:45:00 EDT 2012
#    copyright            : (C) 2012 Owen Williams
#    email                : owilliams@mixxx.org
#############################################################################


import sys
import getopt

def help():
  print "usage: make_xone [args] outputfilename"
  print "optional args: "
  print "          --4decks     to generate a layout for 4 decks"
  print "          --2decks2fx   to generate a layout for 2 decks and 2 effect units"
  print "          --4fx        to generate a layout for 4 effect units"

if len(sys.argv) < 2:
  help()
  sys.exit(1)

fname = ""
# Layout defines how decks and fx are arranged on the xone.
# the first dimension is the xone channel 0 through 3.
# Each item in the list is a tuple of string and int, where the string is either "deck" or "fx"
# and the int is the channel number
LAYOUT = []
NAME = ''
ID = ''

try:
  opts, args = getopt.getopt(sys.argv[1:], "",["4decks","4fx","2decks2fx"])
  for o, a in opts:
    if o == "--4decks":
      LAYOUT = [('deck', 2), ('deck', 0), ('deck', 1), ('deck', 3)]
      NAME = 'Allen&amp;Heath Xone K2 - 4 decks'
      ID = 'XONE:K2:4D'
    elif o == "--4fx":
      LAYOUT = [('fx', 0), ('fx', 1), ('fx', 2), ('fx', 3)]
      NAME = 'Allen&amp;Heath Xone K2 - 4 fx'
      ID = 'XONE:K2:4FX'
    elif o == "--2decks2fx":
      LAYOUT = [('deck', 0), ('deck', 1), ('fx', 0), ('fx', 1)]
      NAME = 'Allen&amp;Heath Xone K2 - 2 decks 2 fx'
      ID = 'XONE:K2:2D2FX'
  if len(args) > 0:
    fname = args[0]
except Exception, e:
  print str(e)
  help()
  sys.exit(1)

if fname == "":
  help()
  sys.exit(1)

try:
  fh = open(fname, "w")
except Exception, e:
  print "Error opening file for write:", str(e)
  sys.exit(1)

button_definitions = """0x34	0x35	0x36	0x37
0x58	0x59	0x5A	0x5B
0x7C	0x7d	0x7e	0x7f


0x30	0x31	0x32	0x33
0x54	0x55	0x56	0x57
0x78	0x79	0x7A	0x7b



0x2c	0x2d	0x2e	0x2f
0x50	0x51	0x52	0x53
0x74	0x75	0x76	0x77


0x28	0x29	0x2a	0x2b
0x4c	0x4d	0x4e	0x4f
0x70	0x71	0x72	0x73



0x24	0x25	0x26	0x27
0x48	0x49	0x4a	0x4b
0x6c	0x6d	0x6e	0x6f


0x20	0x21	0x22	0x23
0x44	0x45	0x46	0x47
0x68	0x69	0x6a	0x6b


0x1c	0x1d	0x1e	0x1f
0x40	0x41	0x42	0x43
0x64	0x65	0x66	0x67


0x18	0x19	0x1A	0x1b
0x3c	0x3d	0x3e	0x3f
0x60	0x61	0x62	0x63"""

controls = ['spinknob', 'knoblight1', 'knoblight2', 'knoblight3', 'button1', 'button2', 'button3', 'button4']
controls.reverse()
colors = ['red', 'orange', 'green']
cur_color = 0

# {'spinknob': [{'red':1,'orange':2,'green':3},{'red':4,'orange':5,'green':6}...

midi = {}
current_control = controls.pop()
for line in button_definitions.split("\n"):
  if len(line.strip()) == 0:
    continue

  hexlist = line.split()

  if current_control not in midi:
    midi[current_control] = [{},{},{},{}]

  for i,h in enumerate(hexlist):
    midi[current_control][i][colors[cur_color]]=h

  cur_color = (cur_color + 1) % len(colors)
  if cur_color == 0:
    try:
      current_control = controls.pop()
    except:
        break

#ok now we have a mapping between control and hex key

#now we need midi CC's -- these just increment simply

cc_controls=['spinknob', 'eq1', 'eq2', 'eq3', 'slider']
cc_code=0
midi_cc = {}
for cc in cc_controls:
  midi_cc[cc] = []
  for i in range(0, 4):
    midi_cc[cc].append("0x%x" % cc_code)
    cc_code += 1


#now we need a mapping of control and controlobject

#controls = ['spinknob', 'knoblight1', 'knoblight2', 'knoblight3', 'button1', 'button2', 'button3', 'button4']



##################################################
########### Actual stuff you can edit! ###########
##################################################

# Note that EQ control objects are munged in code to produce the correct control objects as of
# Mixxx 2.0.

cc_mapping = {
  'deck': {
    'spinknob':('XoneK2.encoderJog','<Script-Binding/>','ch'),
    'eq1':('parameter3','<normal/>','eq'),
    'eq2':('parameter2','<normal/>','eq'),
    'eq3':('parameter1','<normal/>','eq'),
    'slider':('volume','<normal/>','ch')},
  'fx': {
    'spinknob':('effect_selector','<selectknob/>','fx'),
    'eq1':('parameter3','<normal/>','fx'),
    'eq2':('parameter2','<normal/>','fx'),
    'eq3':('parameter1','<normal/>','fx'),
    'slider':('mix','<normal/>','ufx')},
}

button_mapping = {
  'deck': {
    'spinknob':('XoneK2.encoderButton','<Script-Binding/>', 'ch'),
    'knoblight1':('keylock','<button/>','ch'), 'knoblight2':('quantize','<normal/>', 'ch'), 'knoblight3':('button_parameter1','<normal/>', 'eq'),
    'button1':{'red':('pfl','<button/>','ch'), 'orange':('beatloop_4','<button/>','ch'),  'green':('hotcue_1_activate','<button/>','ch')},
    'button2':{'red':('sync_enabled','<button/>','ch'), 'orange':('loop_double','<button/>','ch'), 'green':('hotcue_2_activate','<button/>','ch')},
    'button3':{'red':('cue_default','<button/>','ch'), 'orange':('loop_halve','<button/>','ch'),  'green':('hotcue_3_activate','<button/>','ch')},
    'button4':{'red':('play','<button/>','ch'), 'orange':('reloop_exit','<button/>','ch'), 'green':('hotcue_4_activate','<button/>','ch')}},
  'fx': {
    'spinknob':('enabled','<button/>', 'fx'),
    'knoblight1':('enabled','<button/>','fx'), 'knoblight2':('quantize','<normal/>', 'ch'), 'knoblight3':('button_parameter1','<normal/>', 'eq'),
    'button1':{'red':('group_channel4_enable','<button/>','fx'), 'orange':('beatloop_4','<button/>','ch'),  'green':('hotcue_1_activate','<button/>','ch')},
    'button2':{'red':('group_channel3_enable','<button/>','fx'), 'orange':('loop_double','<button/>','ch'), 'green':('hotcue_2_activate','<button/>','ch')},
    'button3':{'red':('group_channel2_enable','<button/>','fx'), 'orange':('loop_halve','<button/>','ch'),  'green':('hotcue_3_activate','<button/>','ch')},
    'button4':{'red':('group_channel1_enable','<button/>','fx'), 'orange':('reloop_exit','<button/>','ch'), 'green':('hotcue_4_activate','<button/>','ch')}
  },
}

light_mapping = {
  'deck': {
    #'spinknob':('jog', 'ch'),
    'knoblight1':('keylock', 'ch'), 'knoblight2':('quantize', 'ch'), 'knoblight3':('button_parameter1', 'eq'),
    'button1':{'red':('pfl', 'ch'), 'orange':('beatloop_4', 'ch'), 'green':('hotcue_1_enabled', 'ch')},
    'button2':{'red':('sync_enabled', 'ch'), 'orange':('loop_double', 'ch'), 'green':('hotcue_2_enabled', 'ch')},
    'button3':{'red':('cue_default', 'ch'), 'orange':('loop_halve', 'ch'), 'green':('hotcue_3_enabled', 'ch')},
    'button4':{'red':('play', 'ch'), 'orange':('loop_enabled', 'ch'), 'green':('hotcue_4_enabled', 'ch')}},
  'fx':{
    'knoblight1':('enabled', 'fx'), 'knoblight2':('quantize', 'ch'), 'knoblight3':('button_parameter1', 'eq'),
    'button1':{'red':('group_channel4_enable', 'fx'), 'orange':('beatloop_4', 'ch'), 'green':('hotcue_1_enabled', 'ch')},
    'button2':{'red':('group_channel3_enable', 'fx'), 'orange':('loop_double', 'ch'), 'green':('hotcue_2_enabled', 'ch')},
    'button3':{'red':('group_channel2_enable', 'fx'), 'orange':('loop_halve', 'ch'), 'green':('hotcue_3_enabled', 'ch')},
    'button4':{'red':('group_channel1_enable', 'fx'), 'orange':('loop_enabled', 'ch'), 'green':('hotcue_4_enabled', 'ch')}
  },
}


#these aren't worth automating
master_knobs ="""            <control>
                <group>[Playlist]</group>
                <key>XoneK2.rightBottomKnob</key>
                <status>0xBF</status>
                <midino>0x15</midino>
                <options>
                    <Script-Binding/>
                </options>
            </control>
             <control>
                <group>[Playlist]</group>
                <key>LoadSelectedIntoFirstStopped</key>
                <status>0x9F</status>
                <midino>0x0E</midino>
                <options>
                    <normal/>
                </options>
            </control>
            <control>
                <group>[Master]</group>
                <key>XoneK2.leftBottomKnob</key>
                <status>0xBF</status>
                <midino>0x14</midino>
                <options>
                    <Script-Binding/>
                </options>
            </control>
            <control>
                <group>[Master]</group>
                <key>XoneK2.shift_on</key>
                <status>0x9F</status>
                <midino>0xF</midino>
                <options>
                    <Script-Binding/>
                </options>
            </control>
            <control>
                <group>[Master]</group>
                <key>XoneK2.shift_on</key>
                <status>0x8F</status>
                <midino>0xF</midino>
                <options>
                    <Script-Binding/>
                </options>
            </control>"""


xml = []
xml.append("""<?xml version='1.0' encoding='utf-8'?>
<!-- This file automatically generated by make_xone.py. -->
<MixxxControllerPreset mixxxVersion="" schemaVersion="1">
    <info>
        <name>%s</name>
        <author>Owen Williams</author>
        <description>For this mapping to work:
- Set Xone:K2 midi channel to 16;
- Set Xone:K2 Latching Layers state to "Switch Matrix" (state 2).
(See product manual for details.)</description>
    </info>
    <controller id="%s">
        <scriptfiles>
            <file filename="Xone-K2-scripts.js" functionprefix="XoneK2"/>
        </scriptfiles>
        <controls>""" % (NAME, ID))


xml.append("<!-- CC Controls (knobs and sliders) -->")

########################################
########################################
########################################
# ok back to boilerplate...

def get_group_name(channel, key, cotype):
  """Builds correct group name based on type and channel and key"""
  if cotype == 'eq':
    return "[EqualizerRack1_[Channel%d]_Effect1]" % channel
  elif cotype == 'ch':
    return "[Channel%d]" % channel
  elif cotype == 'fx':
    return "[EffectRack1_EffectUnit1_Effect%d]" % channel
  elif cotype == 'ufx':
    return "[EffectRack1_EffectUnit1]"

#cc controls (no latching needed)
for i, op in enumerate(LAYOUT):
  chantype, channel = op
  ccmap = cc_mapping[chantype]
  for cc in ccmap:
    xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0xBF</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, ccmap[cc][0], ccmap[cc][2]),
                             ccmap[cc][0], midi_cc[cc][i], ccmap[cc][1]))

#Spin Knob buttons (no latching needed)
for i, op in enumerate(LAYOUT):
  chantype, channel = op
  bmap = button_mapping[chantype]
  xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x9F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap['spinknob'][0], bmap['spinknob'][2]),
                             bmap['spinknob'][0], midi['spinknob'][i]['red'], bmap['spinknob'][1]))
  xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x8F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap['spinknob'][0], bmap['spinknob'][2]),
                             bmap['spinknob'][0], midi['spinknob'][i]['red'], bmap['spinknob'][1]))

xml.append("<!-- Upper Buttons -->")
#knoblight buttons (no latching)
for j, op in enumerate(LAYOUT):
  chantype, channel = op
  bmap = button_mapping[chantype]
  for knob in ['knoblight%i' % i for i in range(1,4)]:
    xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x9F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap[knob][0], bmap[knob][2]),
                             bmap[knob][0], midi[knob][j]['red'], bmap[knob][1]))
    xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x8F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap[knob][0], bmap[knob][2]),
                             bmap[knob][0], midi[knob][j]['red'], bmap[knob][1]))

xml.append("<!-- Lower Button Grid -->")

#buttons
for j, op in enumerate(LAYOUT):
  chantype, channel = op
  bmap = button_mapping[chantype]
  for latch in ['red','orange','green']:
    for button in ['button%i' % i for i in range(1,5)]:
      xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x9F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap[button][latch][0], bmap[button][latch][2]),
                             bmap[button][latch][0], midi[button][j][latch], bmap[button][latch][1]))
      xml.append("""            <control>
                <group>%s</group>
                <key>%s</key>
                <status>0x8F</status>
                <midino>%s</midino>
                <options>
                    %s
                </options>
            </control>""" % (get_group_name(channel+1, bmap[button][latch][0], bmap[button][latch][2]),
                             bmap[button][latch][0], midi[button][j][latch], bmap[button][latch][1]))

xml.append("<!-- Special Case Knobs / buttons -->")
# a couple custom entries:
xml.append(master_knobs)


############ done with controls

xml.append("""        </controls>
        <outputs>""")

#ok now the lights
if 'spinknob' in light_mapping:
  for i, op in enumerate(LAYOUT):
    chantype, channel = op
    lmap = light_mapping[chantype]
    xml.append("""            <output>
                  <group>%s</group>
                  <key>%s</key>
                  <status>0x9F</status>
                  <midino>%s</midino>
                  <minimum>0.5</minimum>
              </output>""" % (get_group_name(channel+1, lmap['spinknob'][0], lmap['spinknob'][1]),
                              lmap['spinknob'][0], midi['spinknob'][i]['red']))


xml.append("<!-- Knob lights -->")
#knoblight buttons (no latching)
for j, op in enumerate(LAYOUT):
  chantype, channel = op
  lmap = light_mapping[chantype]
  for knob in ['knoblight%i' % i for i in range(1,4)]:
    xml.append("""            <output>
                <group>%s</group>
                <key>%s</key>
                <status>0x9F</status>
                <midino>%s</midino>
                <minimum>0.5</minimum>
            </output>""" % (get_group_name(channel+1, lmap[knob][0], lmap[knob][1]),
                            lmap[knob][0], midi[knob][j]['red']))

xml.append("<!-- Button lights -->")
#buttons (latched)
for j, op in enumerate(LAYOUT):
  chantype, channel = op
  lmap = light_mapping[chantype]
  for latch in ['red','orange','green']:
    for button in ['button%i' % i for i in range(1,5)]:
      xml.append("""            <output>
                <group>%s</group>
                <key>%s</key>
                <status>0x9F</status>
                <midino>%s</midino>
                <minimum>0.5</minimum>
            </output>""" % (get_group_name(channel+1, lmap[button][latch][0], lmap[button][latch][1]),
                            lmap[button][latch][0], midi[button][j][latch]))

xml.append("""        </outputs>
    </controller>
</MixxxControllerPreset>""")

fh.writelines("\n".join(xml))
fh.close()
