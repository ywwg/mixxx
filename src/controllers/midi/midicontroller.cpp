/**
 * @file midicontroller.cpp
 * @author Sean Pappalardo spappalardo@mixxx.org
 * @date Tue 7 Feb 2012
 * @brief MIDI Controller base class
 *
 */

#include "controllers/midi/midicontroller.h"

#include "controllers/midi/midiutils.h"
#include "controllers/defs_controllers.h"
#include "controlobject.h"
#include "errordialoghandler.h"
#include "playermanager.h"

MidiController::MidiController() : Controller() {
}

MidiController::~MidiController() {
    destroyOutputHandlers();
    // Don't close the device here. Sub-classes should close the device in their
    // destructors.
}

QString MidiController::presetExtension() {
    return MIDI_PRESET_EXTENSION;
}

void MidiController::visit(const MidiControllerPreset* preset) {
    m_preset = *preset;
    emit(presetLoaded(getPreset()));
}

int MidiController::close() {
    destroyOutputHandlers();
    return 0;
}

void MidiController::visit(const HidControllerPreset* preset) {
    Q_UNUSED(preset);
    qWarning() << "ERROR: Attempting to load an HidControllerPreset to a MidiController!";
    // TODO(XXX): throw a hissy fit.
}

bool MidiController::matchPreset(const PresetInfo& preset) {
    // Product info mapping not implemented for MIDI devices yet
    Q_UNUSED(preset);
    return false;
}

bool MidiController::savePreset(const QString fileName) const {
    MidiControllerPresetFileHandler handler;
    return handler.save(m_preset, getName(), fileName);
}

void MidiController::applyPreset(QList<QString> scriptPaths) {
    // Handles the engine
    Controller::applyPreset(scriptPaths);

    // Only execute this code if this is an output device
    if (isOutputDevice()) {
        if (m_outputs.count() > 0) {
            destroyOutputHandlers();
        }
        createOutputHandlers();
        updateAllOutputs();
    }
}

void MidiController::createOutputHandlers() {
    if (m_preset.outputMappings.isEmpty()) {
        return;
    }

    QHashIterator<ConfigKey, MidiOutputMapping> outIt(m_preset.outputMappings);
    QStringList failures;
    while (outIt.hasNext()) {
        outIt.next();

        const MidiOutputMapping& mapping = outIt.value();

        QString group = mapping.control.group;
        QString key = mapping.control.item;

        unsigned char status = mapping.output.status;
        unsigned char control = mapping.output.control;
        unsigned char on = mapping.output.on;
        unsigned char off = mapping.output.off;
        double min = mapping.output.min;
        double max = mapping.output.max;

        if (debugging()) {
            qDebug() << QString(
                "Creating output handler for %1,%2 between %3 and %4 to MIDI out: 0x%5 0x%6, on: 0x%7 off: 0x%8")
                    .arg(group, key,
                            QString::number(min), QString::number(max),
                            QString::number(status, 16).toUpper(),
                            QString::number(control, 16).toUpper().rightJustified(2,'0'),
                            QString::number(on, 16).toUpper().rightJustified(2,'0'),
                            QString::number(off, 16).toUpper().rightJustified(2,'0'));
        }

        MidiOutputHandler* moh = new MidiOutputHandler(this, mapping);
        if (!moh->validate()) {
            QString errorLog =
                QString("MIDI output message 0x%1 0x%2 has invalid MixxxControl %3, %4")
                        .arg(QString::number(status, 16).toUpper(),
                             QString::number(control, 16).toUpper().rightJustified(2,'0'))
                        .arg(group, key).toUtf8();
            qWarning() << errorLog;

            int deckNum = 0;
            if (debugging()) {
                failures.append(errorLog);
            } else if (PlayerManager::isDeckGroup(group, &deckNum)) {
                int numDecks = PlayerManager::numDecks();
                if (deckNum <= numDecks) {
                    failures.append(errorLog);
                }
            }

            delete moh;
            continue;
        }
        m_outputs.append(moh);
    }

    if (!failures.isEmpty()) {
        ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
        props->setType(DLG_WARNING);
        props->setTitle(tr("MixxxControl(s) not found"));
        props->setText(tr("One or more MixxxControls specified in the "
                          "outputs section of the loaded preset were invalid."));
        props->setInfoText(tr("Some LEDs or other feedback may not work correctly."));
        QString detailsText = tr("* Check to see that the MixxxControl "
                                 "names are spelled correctly in the mapping "
                                 "file (.xml)\n");
        detailsText += tr("* Make sure the MixxxControls in question actually exist."
                          " Visit this wiki page for a complete list: ");
        detailsText += "http://mixxx.org/wiki/doku.php/mixxxcontrols\n\n";
        detailsText += failures.join("\n");
        props->setDetails(detailsText);
        ErrorDialogHandler::instance()->requestErrorDialog(props);
    }
}

void MidiController::updateAllOutputs() {
    foreach (MidiOutputHandler* pOutput, m_outputs) {
        pOutput->update();
    }
}

void MidiController::destroyOutputHandlers() {
    while (m_outputs.size() > 0) {
        delete m_outputs.takeLast();
    }
}

QString formatMidiMessage(unsigned char status, unsigned char control, unsigned char value,
                          unsigned char channel, unsigned char opCode) {
    switch (opCode) {
        case MIDI_PITCH_BEND:
            return QString("MIDI status 0x%1: pitch bend ch %2, value 0x%3")
                    .arg(QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((value << 7) | control, 16).toUpper().rightJustified(4,'0'));
        case MIDI_SONG_POS:
            return QString("MIDI status 0x%1: song position 0x%2")
                    .arg(QString::number(status, 16).toUpper(),
                         QString::number((value << 7) | control, 16).toUpper().rightJustified(4,'0'));
        case MIDI_PROGRAM_CH:
        case MIDI_CH_AFTERTOUCH:
            return QString("MIDI status 0x%1 (ch %2, opcode 0x%3), value 0x%4")
                    .arg(QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((status & 255)>>4, 16).toUpper(),
                         QString::number(control, 16).toUpper().rightJustified(2,'0'));
        case MIDI_SONG:
            return QString("MIDI status 0x%1: select song #%2")
                    .arg(QString::number(status, 16).toUpper(),
                         QString::number(control+1, 10));
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTERTOUCH:
        case MIDI_CC:
            return QString("MIDI status 0x%1 (ch %2, opcode 0x%3), ctrl 0x%4, val 0x%5")
                    .arg(QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((status & 255)>>4, 16).toUpper(),
                         QString::number(control, 16).toUpper().rightJustified(2,'0'),
                         QString::number(value, 16).toUpper().rightJustified(2,'0'));
        default:
            return QString("MIDI status 0x%1")
                    .arg(QString::number(status, 16).toUpper());
    }
}

void MidiController::learnTemporaryInputMappings(const MidiInputMappings& mappings) {
    foreach (const MidiInputMapping& mapping, mappings) {
        m_temporaryInputMappings.insert(mapping.key.key, mapping);

        unsigned char opCode = MidiUtils::opCodeFromStatus(mapping.key.status);
        bool twoBytes = MidiUtils::isMessageTwoBytes(opCode);
        QString message = twoBytes ? QString("0x%1 0x%2")
                .arg(QString::number(mapping.key.status, 16).toUpper(),
                     QString::number(mapping.key.control, 16).toUpper()
                     .rightJustified(2,'0')) :
                QString("0x%1")
                .arg(QString::number(mapping.key.status, 16).toUpper());
        qDebug() << "Set mapping for" << message << "to"
                 << mapping.control.group << mapping.control.item;
    }
}

void MidiController::clearTemporaryInputMappings() {
    m_temporaryInputMappings.clear();
}

void MidiController::commitTemporaryInputMappings() {
    // We want to replace duplicates that exist in m_preset but allow duplicates
    // in m_temporaryInputMappings. To do this, we first remove every key in
    // m_temporaryInputMappings from m_preset.inputMappings.
    for (QHash<uint16_t, MidiInputMapping>::const_iterator it =
                 m_temporaryInputMappings.begin();
         it != m_temporaryInputMappings.end(); ++it) {
        m_preset.inputMappings.remove(it.key());
    }

    // Now, we can just use unite since we manually removed the duplicates in
    // the original set.
    m_preset.inputMappings.unite(m_temporaryInputMappings);
    m_temporaryInputMappings.clear();
}

void MidiController::receive(unsigned char status, unsigned char control,
                             unsigned char value) {
    unsigned char channel = MidiUtils::channelFromStatus(status);
    unsigned char opCode = MidiUtils::opCodeFromStatus(status);

    if (debugging()) {
        qDebug() << formatMidiMessage(status, control, value, channel, opCode);
    }

    //if (m_bReceiveInhibit) return;

    MidiKey mappingKey = MidiUtils::makeMidiKey(status, control);

    if (isLearning()) {
        emit(messageReceived(status, control, value));

        QHash<uint16_t, MidiInputMapping>::const_iterator it =
                m_temporaryInputMappings.find(mappingKey.key);
        if (it != m_temporaryInputMappings.end()) {
            for (; it != m_temporaryInputMappings.end() && it.key() == mappingKey.key; ++it) {
                processInputMapping(it.value(), status, control, value);
            }
            return;
        }
    }

    QHash<uint16_t, MidiInputMapping>::const_iterator it =
            m_preset.inputMappings.find(mappingKey.key);
    for (; it != m_preset.inputMappings.end() && it.key() == mappingKey.key; ++it) {
        processInputMapping(it.value(), status, control, value);
    }
}

void MidiController::processInputMapping(const MidiInputMapping& mapping,
                                         unsigned char status,
                                         unsigned char control,
                                         unsigned char value) {
    unsigned char channel = MidiUtils::channelFromStatus(status);
    unsigned char opCode = MidiUtils::opCodeFromStatus(status);

    if (mapping.options.script) {
        ControllerEngine* pEngine = getEngine();
        if (pEngine == NULL) {
            return;
        }

        QScriptValueList args;
        args << QScriptValue(channel);
        args << QScriptValue(control);
        args << QScriptValue(value);
        args << QScriptValue(status);
        args << QScriptValue(mapping.control.group);
        QScriptValue function = pEngine->resolveFunction(
            mapping.control.item, true);
        pEngine->execute(function, args);
        return;
    }

    // Only pass values on to valid ControlObjects.
    ControlObject* pCO = ControlObject::getControl(mapping.control);
    if (pCO == NULL) {
        return;
    }

    double newValue = value;

    //qDebug() << "MIDI Options" << QString::number(mapping.options.all, 2).rightJustified(16,'0');

    // compute 14-bit number for pitch bend messages
    if (opCode == MIDI_PITCH_BEND) {
        int ivalue;
        ivalue = (value << 7) | control;

        // Range is 0x0000..0x3FFF center @ 0x2000, i.e. 0..16383 center @ 8192
        if (mapping.options.invert) {
            newValue = 0x2000-ivalue;
            if (newValue < 0) newValue--;
        } else {
            newValue = ivalue-0x2000;
            if (newValue > 0) newValue++;
        }
        // TODO: use getMin() and getMax() to make this divisor work with all COs
        newValue /= 0x2000; // FIXME: hard-coded for -1.0..1.0

        // computeValue not (yet) done on pitch messages because it all assumes 7-bit numbers
    } else {
        double currControlValue = pCO->getMidiParameter();
        newValue = computeValue(mapping.options, currControlValue, value);
    }

    // ControlPushButton ControlObjects only accept NOTE_ON, so if the midi
    // mapping is <button> we override the Midi 'status' appropriately.
    if (mapping.options.button || mapping.options.sw) {
        opCode = MIDI_NOTE_ON;
    }

    if (mapping.options.soft_takeover) {
        // This is the only place to enable it if it isn't already.
        m_st.enable(pCO);
    }

    if (opCode == MIDI_PITCH_BEND) {
        // Absolute value is calculated above on Pitch messages (-1..1)
        if (mapping.options.soft_takeover) {
            if (m_st.ignore(pCO, newValue, false)) {
                return;
            }
        }
        // Use temporary cot for bypass own signal filter
        ControlObjectThread cot(pCO->getKey());
        cot.set(newValue);
    } else {
        if (mapping.options.soft_takeover) {
            if (m_st.ignore(pCO, newValue, true)) {
                return;
            }
        }
        pCO->setValueFromMidi(static_cast<MidiOpCode>(opCode), newValue);
    }
}

double MidiController::computeValue(MidiOptions options, double _prevmidivalue, double _newmidivalue) {
    double tempval = 0.;
    double diff = 0.;

    if (options.all == 0) {
        return _newmidivalue;
    }

    if (options.invert) {
        return 127. - _newmidivalue;
    }

    if (options.rot64 || options.rot64_inv) {
        tempval = _prevmidivalue;
        diff = _newmidivalue - 64.;
        if (diff == -1 || diff == 1)
            diff /= 16;
        else
            diff += (diff > 0 ? -1 : +1);
        if (options.rot64)
            tempval += diff;
        else
            tempval -= diff;
        return (tempval < 0. ? 0. : (tempval > 127. ? 127.0 : tempval));
    }

    if (options.rot64_fast) {
        tempval = _prevmidivalue;
        diff = _newmidivalue - 64.;
        diff *= 1.5;
        tempval += diff;
        return (tempval < 0. ? 0. : (tempval > 127. ? 127.0 : tempval));
    }

    if (options.diff) {
        //Interpret 7-bit signed value using two's compliment.
        if (_newmidivalue >= 64.)
            _newmidivalue = _newmidivalue - 128.;
        //Apply sensitivity to signed value. FIXME
       // if(sensitivity > 0)
        //    _newmidivalue = _newmidivalue * ((double)sensitivity / 50.);
        //Apply new value to current value.
        _newmidivalue = _prevmidivalue + _newmidivalue;
    }

    if (options.selectknob) {
        //Interpret 7-bit signed value using two's compliment.
        if (_newmidivalue >= 64.)
            _newmidivalue = _newmidivalue - 128.;
        //Apply sensitivity to signed value. FIXME
        //if(sensitivity > 0)
        //    _newmidivalue = _newmidivalue * ((double)sensitivity / 50.);
        //Since this is a selection knob, we do not want to inherit previous values.
    }

    if (options.button) {
        _newmidivalue = _newmidivalue != 0;
    }

    if (options.sw) {
        _newmidivalue = 1;
    }

    if (options.spread64) {
        //qDebug() << "MIDI_OPT_SPREAD64";
        // BJW: Spread64: Distance away from centre point (aka "relative CC")
        // Uses a similar non-linear scaling formula as ControlTTRotary::getValueFromWidget()
        // but with added sensitivity adjustment. This formula is still experimental.

        _newmidivalue = _newmidivalue - 64.;
        //FIXME
        //double distance = _newmidivalue - 64.;
        // _newmidivalue = distance * distance * sensitivity / 50000.;
        //if (distance < 0.)
        //    _newmidivalue = -_newmidivalue;

        //qDebug() << "Spread64: in " << distance << "  out " << _newmidivalue;
    }

    if (options.herc_jog) {
        if (_newmidivalue > 64.) {
            _newmidivalue -= 128.;
        }
        _newmidivalue += _prevmidivalue;
        //if (_prevmidivalue != 0.0) { qDebug() << "AAAAAAAAAAAA" << _prevmidivalue; }
    }

    return _newmidivalue;
}

QString formatSysexMessage(QString controllerName, const QByteArray& data) {
    QString message = QString("%1: %2 bytes: [").arg(controllerName).arg(data.size());
    for (int i = 0; i < data.size(); ++i) {
        message += QString("%1%2").arg(
            QString("%1").arg((unsigned char)(data.at(i)), 2, 16, QChar('0')).toUpper(),
            QString("%1").arg((i < (data.size()-1)) ? ' ' : ']'));
    }
    return message;
}

void MidiController::receive(QByteArray data) {
    if (debugging()) {
        qDebug() << formatSysexMessage(getName(), data);
    }

    //if (m_bReceiveInhibit) return;

    MidiKey mappingKey = MidiUtils::makeMidiKey(data.at(0), 0xFF);

    // TODO(rryan): Need to review how MIDI learn works with sysex messages. I
    // don't think this actually does anything useful.
    if (isLearning()) {
        // TODO(rryan): Fake a one value?
        emit(messageReceived(mappingKey.status, mappingKey.control, 0x7F));

        QHash<uint16_t, MidiInputMapping>::const_iterator it =
                m_temporaryInputMappings.find(mappingKey.key);
        if (it != m_temporaryInputMappings.end()) {
            for (; it != m_temporaryInputMappings.end() && it.key() == mappingKey.key; ++it) {
                processInputMapping(it.value(), data);
            }
            return;
        }
    }

    QHash<uint16_t, MidiInputMapping>::const_iterator it =
            m_preset.inputMappings.find(mappingKey.key);
    for (; it != m_preset.inputMappings.end() && it.key() == mappingKey.key; ++it) {
        processInputMapping(it.value(), data);
    }
}

void MidiController::processInputMapping(const MidiInputMapping& mapping,
                                         const QByteArray& data) {
    // Custom script handler
    if (mapping.options.script) {
        ControllerEngine* pEngine = getEngine();
        if (pEngine == NULL) {
            return;
        }
        QScriptValue function = pEngine->resolveFunction(mapping.control.item, true);
        if (!pEngine->execute(function, data)) {
            qDebug() << "MidiController: Invalid script function" << mapping.control.item;
        }
        return;
    }
    qWarning() << "MidiController: No script function specified for"
               << formatSysexMessage(getName(), data);
}

void MidiController::sendShortMsg(unsigned char status, unsigned char byte1,
                                  unsigned char byte2) {
    unsigned int word = (((unsigned int)byte2) << 16) |
            (((unsigned int)byte1) << 8) | status;
    sendWord(word);
}
