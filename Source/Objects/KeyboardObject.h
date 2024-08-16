/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Inherit to customise drawing
class MIDIKeyboard : public MidiKeyboardState, public MidiKeyboardComponent
{
    bool toggleMode = false;
    int lastKey = -1;

public:
    int clickedKey = -1;

    std::set<int> heldKeys;
    std::set<int> toggledKeys;
    std::function<void(int, int)> noteOn;
    std::function<void(int)> noteOff;
    
    Canvas* cnv;
    PluginEditor* editor;

    MIDIKeyboard(Canvas* cnv)
        : MidiKeyboardComponent(*this, MidiKeyboardComponent::horizontalKeyboard)
        , cnv(cnv)
        , editor(cnv->editor)
    {
        // Make sure nothing is drawn outside of our custom draw functions
        setColour(MidiKeyboardComponent::whiteNoteColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keyDownOverlayColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::textLabelColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::shadowColourId, Colours::transparentBlack);
    }

    /*  Return the amount of white notes in the current displayed range.
     *  We use this to calculate & resize the keyboard width when more range is added
     *  because setKeyWidth sets the width of white keys
     */
    int getCountOfWhiteNotesInRange()
    {
        /*
         ┌──┬─┬─┬─┬──┬──┬─┬─┬─┬─┬─┬──┐
         │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
         │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
         │  └┼┘ └┼┘  │  └┼┘ └┼┘ └┼┘  │
         │ 0 │ 2 │ 4 │ 5 │ 7 │ 9 │11 │
         └───┴───┴───┴───┴───┴───┴───┘
         */
        int count = 0;
        for (int i = getRangeStart(); i <= getRangeEnd(); i++) {
            if (i % 12 == 0 || i % 12 == 2 || i % 12 == 4 || i % 12 == 5 || i % 12 == 7 || i % 12 == 9 || i % 12 == 11) {
                count++;
            }
        }
        return count;
    }

    bool mouseDownOnKey(int midiNoteNumber, MouseEvent const& e) override
    {
        clickedKey = midiNoteNumber;

        if (e.mods.isShiftDown()) {
            if (toggledKeys.count(midiNoteNumber)) {
                toggledKeys.erase(midiNoteNumber);
                noteOff(midiNoteNumber);
            } else {
                toggledKeys.insert(midiNoteNumber);
                noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);
            }
        } else if (toggleMode) {
            if (heldKeys.count(midiNoteNumber)) {
                heldKeys.erase(midiNoteNumber);
                noteOff(midiNoteNumber);
            } else {
                heldKeys.insert(midiNoteNumber);
                lastKey = midiNoteNumber;
                noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);
            }
        } else {
            heldKeys.insert(midiNoteNumber);
            lastKey = midiNoteNumber;
            noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);
        }

        repaint();
        return false;
    }

    void resetToggledKeys()
    {
        for (auto key : toggledKeys){
            noteOff(key);
        }
        toggledKeys.clear();
        repaint();
    }

    bool mouseDraggedToKey(int midiNoteNumber, MouseEvent const& e) override
    {
        clickedKey = midiNoteNumber;

        if (!toggleMode && !e.mods.isShiftDown() && !heldKeys.count(midiNoteNumber)) {
            for (auto& note : heldKeys) {
                noteOff(note);
            }
            if (lastKey != midiNoteNumber) {
                heldKeys.erase(lastKey);
            }

            lastKey = midiNoteNumber;

            heldKeys.insert(midiNoteNumber);
            noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);

            repaint();
        }

        return true;
    }

    // When dragging over the keyboard, the cursor may leave the keyboard object.
    // If the user ends the drag action (mouse up) when not over the keyboard object,
    // the keyboard will not register the mouse up, and the key will be stuck on.
    // This could possibly be a bug in juce.
    // So we completely replace mouseUpOnKey functionality here, mouseUp() will stop mouseUpOnKey() being called.
    void mouseUp(MouseEvent const& e) override
    {
        clickedKey = -1;

        if (!toggleMode && !e.mods.isShiftDown()) {
            heldKeys.erase(lastKey);
            noteOff(lastKey);
        }
        repaint();
    }

    void setToggleMode(bool enableToggleMode)
    {
        toggleMode = enableToggleMode;
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver) override
    {
        isDown = heldKeys.count(midiNoteNumber) || toggledKeys.count(midiNoteNumber);

        auto& lnf = editor->getLookAndFeel();
        area = area.reduced(0.0f, 1.0f);

        // Rounded first and last keys to fix objects
        auto* nvg = editor->nvgSurface.getRawContext();
        if(!nvg) return;
        
        if (isOver || isDown) {
            auto c = isDown ? lnf.findColour(PlugDataColour::dataColourId) : Colour(235, 235, 235);
            nvgFillColor(nvg, NVGComponent::convertColour(c));
            if (midiNoteNumber == getRangeStart()) {
                nvgBeginPath(nvg);
                nvgRoundedRectVarying(nvg, area.getX(), area.getY(), area.getWidth(), area.getHeight(), Corners::objectCornerRadius, 0, 0, Corners::objectCornerRadius);
                nvgFill(nvg);
            } else if (midiNoteNumber == getRangeEnd()) {
                nvgBeginPath(nvg);
                nvgRoundedRectVarying(nvg, area.getX(), area.getY(), area.getWidth(), area.getHeight(), 0, Corners::objectCornerRadius, Corners::objectCornerRadius, 0);
                nvgFill(nvg);
            } else {
                nvgFillRect(nvg, area.getX(), area.getY(), area.getWidth(), area.getHeight());
            }
        }

        // don't draw the first separator line to fix object look
        if (midiNoteNumber != getRangeStart()) {
            auto const outlineColour = lnf.findColour(PlugDataColour::outlineColourId);
            nvgFillColor(nvg, NVGComponent::convertColour(outlineColour));
            nvgFillRect(nvg, area.getX(), area.getY(), 1, area.getHeight());
        }

        // FIXME: have a unified way to detect when mode changes outside of render callback
        if (cnv->locked.getValue() || cnv->editor->isInPluginMode())
            return;

        // draw C octave numbers
        if (!(midiNoteNumber % 12)) {
            auto text = String(floor(midiNoteNumber / 12) - 1);
            auto rectangle = area.withTrimmedTop(area.proportionOfHeight(0.8f)).reduced(area.getWidth() / 6.0f);
            nvgFillColor(nvg, nvgRGB(90, 90, 90));
            nvgTextAlign(nvg, NVG_ALIGN_CENTER);
            nvgFontSize(nvg, 13);
            nvgText(nvg, rectangle.getCentreX(), rectangle.getCentreY() + 4, text.toRawUTF8(), nullptr);
        }
    }

    void drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver) override
    {
        auto& lnf = editor->getLookAndFeel();
        auto* nvg = editor->nvgSurface.getRawContext();
        if(!nvg) return;
        
        NVGcolor c = nvgRGB(90, 90, 90);

        isDown = heldKeys.count(midiNoteNumber) || toggledKeys.count(midiNoteNumber);

        if (isOver)
            c = nvgRGB(101, 101, 101);
        if (isDown)
            c = NVGComponent::convertColour(lnf.findColour(PlugDataColour::dataColourId).darker(0.5f));

        nvgFillColor(nvg, c);
        nvgFillRect(nvg, area.getX(), area.getY() + 1.0f, area.getWidth(), area.getHeight() - 1.0f);
    }
};
// ELSE keyboard
class KeyboardObject final : public ObjectBase
    , public Timer {

    Value lowC = SynchronousValue();
    Value octaves = SynchronousValue();
    int numWhiteKeys = 8;

    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value toggleMode = SynchronousValue();
    Value sizeProperty = SynchronousValue();

    MIDIKeyboard keyboard;
    int keyRatio = 5;

    std::unique_ptr<NanoVGGraphicsContext> nvgCtx = nullptr;

public:
    KeyboardObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object), keyboard(object->cnv)
    {
        keyboard.setMidiChannel(1);
        keyboard.setScrollButtonsVisible(false);

        keyboard.noteOn = [this](int note, int velocity) {
            int ac = 2;
            t_atom at[2];
            SETFLOAT(at, note);
            SETFLOAT(at + 1, velocity);

            if (auto obj = this->ptr.get<t_fake_keyboard>()) {
                outlet_list(obj->x_out, gensym("list"), ac, at);
                if (obj->x_send != gensym("") && obj->x_send->s_thing)
                    pd_list(obj->x_send->s_thing, gensym("list"), ac, at);
            }
        };

        keyboard.noteOff = [this](int note) {
            if (auto obj = this->ptr.get<t_fake_keyboard>()) {
                int ac = 2;
                t_atom at[2];
                SETFLOAT(at, note);
                SETFLOAT(at + 1, 0);

                outlet_list(obj->x_out, gensym("list"), ac, at);
                if (obj->x_send != gensym("") && obj->x_send->s_thing)
                    pd_list(obj->x_send->s_thing, gensym("list"), ac, at);
            }
        };

        addAndMakeVisible(keyboard);

        objectParameters.addParamInt("Height", cDimensions, &sizeProperty);
        objectParameters.addParamInt("Start octave", cGeneral, &lowC, 2);
        objectParameters.addParamInt("Num. octaves", cGeneral, &octaves, 4);
        objectParameters.addParamBool("Toggle Mode", cGeneral, &toggleMode, { "Off", "On" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);

        startTimer(50);
    }

    void update() override
    {
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            lowC.setValue(obj->x_low_c);
            octaves.setValue(obj->x_octaves);
            toggleMode.setValue(obj->x_toggle_mode);
            sizeProperty.setValue(obj->x_height);

            auto sndSym = obj->x_snd_set ? String::fromUTF8(obj->x_snd_raw->s_name) : getBinbufSymbol(7);
            auto rcvSym = obj->x_rcv_set ? String::fromUTF8(obj->x_rcv_raw->s_name) : getBinbufSymbol(8);

            sendSymbol = sndSym != "empty" ? sndSym : "";
            receiveSymbol = rcvSym != "empty" ? rcvSym : "";

            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    _this->updateAspectRatio();
                    // Call async to make sure pd obj has updated
                    _this->object->updateBounds();
                }
            });
        }

        keyboard.setToggleMode(getValue<bool>(toggleMode));
    }

    void render(NVGcontext* nvg) override
    {
        if (!nvgCtx || nvgCtx->getContext() != nvg)
            nvgCtx = std::make_unique<NanoVGGraphicsContext>(nvg);
        
        auto b = getLocalBounds();
        
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId));
        
        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBA(225, 225, 225, 255), outlineColour, Corners::objectCornerRadius);
        
        Graphics g(*nvgCtx);
        {
            NVGScopedState scopedState(nvg);
            paintEntireComponent(g, true);
        }
        
        //nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(Colours::transparentBlack), outlineColour, Corners::objectCornerRadius);
    }

    void updateSizeProperty() override
    {
        if (auto keyboard = ptr.get<t_fake_keyboard>()) {
            setParameterExcludingListener(sizeProperty, object->getObjectBounds().getHeight());
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x, y, w, h;
            pd::Interface::getObjectBounds(patch, obj.cast<t_gobj>(), &x, &y, &w, &h);

            return Rectangle<int>(x, y, obj->x_space * numWhiteKeys, obj->x_height);
        }

        return {};
    }

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto gobj = ptr.get<t_fake_keyboard>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            pd::Interface::moveObject(patch, gobj.cast<t_gobj>(), b.getX(), b.getY());
            gobj->x_height = b.getHeight();
        }
    }

    void resized() override
    {
        float keyWidth = static_cast<float>(object->getHeight() - Object::doubleMargin) / keyRatio;

        if (keyWidth <= 0)
            return;

        keyboard.setKeyWidth(keyWidth);

        if (auto obj = ptr.get<t_fake_keyboard>()) {
            obj->x_space = keyWidth;
        }

        keyboard.setSize(keyWidth * numWhiteKeys, object->getHeight() - Object::doubleMargin);
    }

    void updateAspectRatio()
    {
        int numOctaves = getValue<int>(octaves);
        int lowest = getValue<int>(lowC);
        int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
        keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12) - 1, 127));

        float horizontalLength = keyboard.getTotalKeyboardWidth();

        // we only need to get the amount of white notes when the number of keys has changed
        numWhiteKeys = keyboard.getCountOfWhiteNotesInRange();

        object->setSize(horizontalLength + Object::doubleMargin, object->getHeight());
        constrainer->setFixedAspectRatio(horizontalLength / static_cast<float>(object->getHeight() - Object::doubleMargin));
        constrainer->setMinimumSize((object->minimumSize / 5.0f) * numWhiteKeys, object->minimumSize);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto height = std::max(getValue<int>(sizeProperty), constrainer->getMinimumHeight());
            setParameterExcludingListener(sizeProperty, height);
            if (auto keyboard = ptr.get<t_fake_keyboard>()) {
                keyboard->x_height = height;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(lowC)) {
            lowC = std::clamp<int>(getValue<int>(lowC), -1, 9);
            if (auto obj = ptr.get<t_fake_keyboard>())
                obj->x_low_c = getValue<int>(lowC);
            updateAspectRatio();
        } else if (value.refersToSameSourceAs(octaves)) {
            octaves = std::clamp<int>(getValue<int>(octaves), 1, 11);
            if (auto obj = ptr.get<t_fake_keyboard>())
                obj->x_octaves = getValue<int>(octaves);
            updateAspectRatio();
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "send", { pd->generateSymbol(symbol) });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "receive", { pd->generateSymbol(symbol) });
        } else if (value.refersToSameSourceAs(toggleMode)) {
            auto toggle = getValue<int>(toggleMode);
            if (auto obj = ptr.get<void>())
                pd->sendDirectMessage(obj.get(), "toggle", { (float)toggle });
            keyboard.setToggleMode(toggle);
        }
    }

    void updateValue()
    {
        int notes[256];
        if (auto obj = ptr.get<t_fake_keyboard>()) {
            memcpy(notes, obj->x_tgl_notes, 256 * sizeof(int));
        }

        for (int i = keyboard.getRangeStart(); i <= keyboard.getRangeEnd(); i++) {
            if (notes[i] && !keyboard.heldKeys.contains(i)) {
                keyboard.heldKeys.insert(i);
                repaint();
            }
            if (!notes[i] && keyboard.heldKeys.contains(i) && keyboard.clickedKey != i && !getValue<bool>(toggleMode)) {
                keyboard.heldKeys.erase(i);
                repaint();
            }
        }
    }

    void noteOn(int midiNoteNumber, bool isOn)
    {
        if (isOn)
            keyboard.heldKeys.insert(midiNoteNumber);
        else
            keyboard.heldKeys.erase(midiNoteNumber);

        keyboard.repaint();
    }

    void notesOn(pd::Atom const atoms[8], int numAtoms, bool isOn)
    {
        for (int at = 0; at < numAtoms; at++) {
            if (isOn)
                keyboard.heldKeys.insert(atoms[at].getFloat());
            else
                keyboard.heldKeys.erase(atoms[at].getFloat());
        }
        keyboard.repaint();
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        auto elseKeyboard = ptr.get<t_fake_keyboard>();

        switch (symbol) {
        case hash("float"): {
            auto note = std::clamp<int>(atoms[0].getFloat(), 0, 128);
            noteOn(atoms[0].getFloat(), elseKeyboard->x_tgl_notes[note]);
            break;
        }
        case hash("list"): {
            if (numAtoms == 2) {
                noteOn(atoms[0].getFloat(), atoms[1].getFloat() > 0);
            }
            break;
        }
        case hash("set"): {
            // not implemented yet
            break;
        }
        case hash("on"): {
            notesOn(atoms, numAtoms, true);
            break;
        }
        case hash("off"): {
            notesOn(atoms, numAtoms, false);
            break;
        }
        case hash("lowc"): {
            setParameterExcludingListener(lowC, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        case hash("oct"): {
            setParameterExcludingListener(lowC, std::clamp<int>(getValue<int>(lowC) + static_cast<int>(atoms[0].getFloat()), -1, 9));
            updateAspectRatio();
            break;
        }
        case hash("8ves"): {
            setParameterExcludingListener(octaves, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        case hash("send"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("toggle"): {
            setParameterExcludingListener(toggleMode, atoms[0].getFloat());
            keyboard.setToggleMode(static_cast<bool>(atoms[0].getFloat()));
        }
        case hash("flush"): {
            // It's not clear if flush should only clear active toggled notes, or all notes off also?
            // Let's do both to be safe
            keyboard.allNotesOff(0);
            keyboard.resetToggledKeys();
        }
        default:
            break;
        }
    }

    bool inletIsSymbol() override
    {
        auto rSymbol = receiveSymbol.toString();
        return rSymbol.isNotEmpty() && (rSymbol != "empty");
    }

    bool outletIsSymbol() override
    {
        auto sSymbol = sendSymbol.toString();
        return sSymbol.isNotEmpty() && (sSymbol != "empty");
    }

    void timerCallback() override
    {
        updateValue();
    }
};
