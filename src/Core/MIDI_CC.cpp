#include "Core.hpp"


struct MIDI_CC : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CC_OUTPUT, 16),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	midi::InputQueue midiInput;
	int8_t values[128];
	int learningId = -1;
	int learnedCcs[16] = {};
	dsp::ExponentialFilter valueFilters[16];
	int8_t lastValues[16] = {};

	MIDI_CC() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
	}

	void onReset() override {
		for (int i = 0; i < 128; i++) {
			values[i] = 0;
		}
		for (int i = 0; i < 16; i++) {
			learnedCcs[i] = i;
		}
		learningId = -1;
		midiInput.reset();
	}

	void step() override {
		midi::Message msg;
		while (midiInput.shift(&msg)) {
			processMessage(msg);
		}

		float lambda = app()->engine->getSampleTime() * 100.f;
		for (int i = 0; i < 16; i++) {
			if (!outputs[CC_OUTPUT + i].isActive())
				continue;

			int cc = learnedCcs[i];

			float value = rescale(values[cc], 0, 127, 0.f, 10.f);
			valueFilters[i].lambda = lambda;

			// Detect behavior from MIDI buttons.
			if ((lastValues[i] == 0 && values[cc] == 127) || (lastValues[i] == 127 && values[cc] == 0)) {
				// Jump value
				valueFilters[i].out = value;
			}
			else {
				// Smooth value with filter
				valueFilters[i].process(value);
			}
			lastValues[i] = values[cc];
			outputs[CC_OUTPUT + i].setVoltage(valueFilters[i].out);
		}
	}

	void processMessage(midi::Message msg) {
		switch (msg.getStatus()) {
			// cc
			case 0xb: {
				uint8_t cc = msg.getNote();
				// Learn
				if (learningId >= 0 && values[cc] != msg.data2) {
					learnedCcs[learningId] = cc;
					learningId = -1;
				}
				// Allow CC to be negative if the 8th bit is set.
				// The gamepad driver abuses this, for example.
				values[cc] = msg.data2;
			} break;
			default: break;
		}
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		json_t *ccsJ = json_array();
		for (int i = 0; i < 16; i++) {
			json_array_append_new(ccsJ, json_integer(learnedCcs[i]));
		}
		json_object_set_new(rootJ, "ccs", ccsJ);

		// Remember values so users don't have to touch MIDI controller knobs when restarting Rack
		json_t *valuesJ = json_array();
		for (int i = 0; i < 128; i++) {
			json_array_append_new(valuesJ, json_integer(values[i]));
		}
		json_object_set_new(rootJ, "values", valuesJ);

		json_object_set_new(rootJ, "midi", midiInput.toJson());
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *ccsJ = json_object_get(rootJ, "ccs");
		if (ccsJ) {
			for (int i = 0; i < 16; i++) {
				json_t *ccJ = json_array_get(ccsJ, i);
				if (ccJ)
					learnedCcs[i] = json_integer_value(ccJ);
			}
		}

		json_t *valuesJ = json_object_get(rootJ, "values");
		if (valuesJ) {
			for (int i = 0; i < 128; i++) {
				json_t *valueJ = json_array_get(valuesJ, i);
				if (valueJ) {
					values[i] = json_integer_value(valueJ);
				}
			}
		}

		json_t *midiJ = json_object_get(rootJ, "midi");
		if (midiJ)
			midiInput.fromJson(midiJ);
	}
};


struct MIDI_CCWidget : ModuleWidget {
	MIDI_CCWidget(MIDI_CC *module) {
		setModule(module);
		setPanel(SVG::load(asset::system("res/Core/MIDI-CC.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addOutput(createOutput<PJ301MPort>(mm2px(Vec(3.894335, 73.344704)), module, MIDI_CC::CC_OUTPUT + 0));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(15.494659, 73.344704)), module, MIDI_CC::CC_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.094982, 73.344704)), module, MIDI_CC::CC_OUTPUT + 2));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(38.693932, 73.344704)), module, MIDI_CC::CC_OUTPUT + 3));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(3.8943355, 84.945023)), module, MIDI_CC::CC_OUTPUT + 4));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(15.49466, 84.945023)), module, MIDI_CC::CC_OUTPUT + 5));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.094982, 84.945023)), module, MIDI_CC::CC_OUTPUT + 6));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(38.693932, 84.945023)), module, MIDI_CC::CC_OUTPUT + 7));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(3.8943343, 96.543976)), module, MIDI_CC::CC_OUTPUT + 8));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(15.494659, 96.543976)), module, MIDI_CC::CC_OUTPUT + 9));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.09498, 96.543976)), module, MIDI_CC::CC_OUTPUT + 10));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(38.693932, 96.543976)), module, MIDI_CC::CC_OUTPUT + 11));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(3.894335, 108.14429)), module, MIDI_CC::CC_OUTPUT + 12));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(15.49466, 108.14429)), module, MIDI_CC::CC_OUTPUT + 13));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(27.09498, 108.14429)), module, MIDI_CC::CC_OUTPUT + 14));
		addOutput(createOutput<PJ301MPort>(mm2px(Vec(38.693932, 108.14429)), module, MIDI_CC::CC_OUTPUT + 15));

		typedef Grid16MidiWidget<CcChoice<MIDI_CC>> TMidiWidget;
		TMidiWidget *midiWidget = createWidget<TMidiWidget>(mm2px(Vec(3.399621, 14.837339)));
		midiWidget->box.size = mm2px(Vec(44, 54.667));
		if (module)
			midiWidget->midiIO = &module->midiInput;
		midiWidget->setModule(module);
		addChild(midiWidget);
	}
};


// Use legacy slug for compatibility
Model *modelMIDI_CC = createModel<MIDI_CC, MIDI_CCWidget>("MIDICCToCVInterface");
