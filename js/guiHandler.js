/*
*
* Red Pitaya scope application
*
* (c) Red Pitaya http://www.redpitaya.com
*
* Gestion des évènements sur l'interface graphique
*/


(function(APP, $, unused) {
	APP.UI = {};

	APP.UI.change_app_run = function(event) {
		var val = APP.updateBoolVal("APP_RUN");
		$("#APP_RUN").text(val? "RUN":"STOP");
		$("#APP_RUN").css("background-color", val? "green":"red");
	}

	APP.UI.change_osc_show = function(event) {
		APP.updateBoolVal("OSC_SHOW_CH", APP.vars.currentChannel, "checked");
	}

	APP.UI.change_couplig = function(event) {
		APP.updateIntVal("OSC_COUPLING_CH", APP.vars.currentChannel);
	}

	APP.UI.change_osc_invert = function(event) {
		APP.updateBoolVal("OSC_INVERT_CH", APP.vars.currentChannel, "checked");
	}

	APP.UI.change_osc_probe = function(event) {
		APP.updateIntVal("OSC_PROBE_CH", APP.vars.currentChannel);
	}

	APP.UI.change_osc_gain = function(event) {
		APP.updateIntVal("OSC_GAIN_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_enable = function(event) {
		APP.updateBoolVal("GEN_ENABLE_CH", APP.vars.currentChannel, "checked");
	}

	APP.UI.change_gen_show = function(event) {
		APP.updateBoolVal("GEN_SHOW_CH", APP.vars.currentChannel, "checked");
	}

	APP.UI.change_gen_frequency = function(event) {
		APP.updateFloatVal("GEN_FREQUENCY_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_amplitude = function(event) {
		APP.updateFloatVal("GEN_AMPLITUDE_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_offset = function(event) {
		APP.updateFloatVal("GEN_OFFSET_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_waveform = function(event) {
		var val = APP.updateIntVal("GEN_WAVEFORM_CH", APP.vars.currentChannel);
		APP.UI.update_gen_waveform_section(val);
	}

	APP.UI.change_gen_mode = function(event) {
		var val = APP.updateIntVal("GEN_MODE_CH", APP.vars.currentChannel);
		APP.UI.update_gen_mode_section(val);
	}

	APP.UI.change_gen_duty_cycle = function(event) {
		APP.updateFloatVal("GEN_DUTY_CYCLE_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_burst_count = function(event) {
		APP.updateIntVal("GEN_BURST_COUNT_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_burst_per = function(event) {
		APP.updateFloatVal("GEN_BURST_PER_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_burst_rep = function(event) {
		APP.updateIntVal("GEN_BURST_REP_CH", APP.vars.currentChannel);
	}

	APP.UI.update_gen_waveform_section = function(waveform) {
		if (waveform == Waveform.PWM) {
			if ($("#duty-cycle-container").is(":hidden"))
				$("#duty-cycle-container").slideDown(300);
		} else if ($("#duty-cycle-container").is(":hidden") == false)
			$("#duty-cycle-container").slideUp(300);
		else
			$("#duty-cycle-container").hide();

		if (waveform == Waveform.PWM || waveform == Waveform.SQUARE) {
			if ($("#rf-time-setup").is(":hidden"))
				$("#rf-time-setup").slideDown(300);
		} else if ($("#time-setup").is(":hidden") == false)
			$("#rf-time-setup").slideUp(300);
		else
			$("#rf-time-setup").hide();
	}

	APP.UI.update_gen_mode_section = function(mode) {

		// On affiche ou on masque les élements graphique permettant de configurer le burst mode
		if (mode == GenMode.BURST) {
			if ($("#burst-setup").is(":hidden"))
				$("#burst-setup").slideDown(300);
		} else if ($("#burst-setup").is(":hidden") == false)
			$("#burst-setup").slideUp(300);
		else
			$("#burst-setup").hide();
	}

	APP.UI.change_gen_impedance = function(mode) {
		APP.updateIntVal("GEN_IMPEDANCE_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_raise_time = function(mode) {
		APP.updateFloatVal("GEN_RISE_TIME_CH", APP.vars.currentChannel);
	}

	APP.UI.change_gen_fall_time = function(mode) {
		APP.updateFloatVal("GEN_FALL_TIME_CH", APP.vars.currentChannel);
	}





	APP.UI.update_osc_menu = function() {
		APP.UI.update_color_bar();
		APP.updateUI("OSC_SHOW_CH", APP.vars.currentChannel, "checked");
		APP.updateUI("OSC_COUPLING_CH", APP.vars.currentChannel);
		APP.updateUI("OSC_INVERT_CH", APP.vars.currentChannel, "checked");
		APP.updateUI("OSC_PROBE_CH", APP.vars.currentChannel);
		APP.updateUI("OSC_GAIN_CH", APP.vars.currentChannel);
	}

	APP.UI.update_gen_menu = function() {
		APP.UI.update_color_bar();
		APP.updateUI("GEN_ENABLE_CH", APP.vars.currentChannel, "checked");
		APP.updateUI("GEN_SHOW_CH", APP.vars.currentChannel, "checked");
		APP.updateUI("GEN_AMPLITUDE_CH", APP.vars.currentChannel);
		APP.updateUI("GEN_FREQUENCY_CH", APP.vars.currentChannel);
		APP.updateUI("GEN_OFFSET_CH", APP.vars.currentChannel);
		APP.UI.update_gen_waveform_section(APP.updateUI("GEN_WAVEFORM_CH", APP.vars.currentChannel));
		APP.UI.update_gen_mode_section(APP.updateUI("GEN_MODE_CH", APP.vars.currentChannel));
		APP.updateUI("GEN_DUTY_CYCLE_CH", APP.vars.currentChannel);
		APP.updateUI("GEN_BURST_COUNT_CH", APP.vars.currentChannel);
		APP.updateUI("GEN_BURST_PER_CH", APP.vars.currentChannel);
		APP.updateUI("GEN_BURST_REP_CH", APP.vars.currentChannel);
	}

	APP.UI.update_math_menu = function() {
		APP.UI.update_color_bar();
		APP.updateUI("MATH_SHOW_CH", APP.vars.currentChannel, "checked");
		APP.updateUI("MATH_OPERATOR_CH", APP.vars.currentChannel);
		APP.updateUI("MATH_FIRST_SIGN_CH", APP.vars.currentChannel);
		APP.updateUI("MATH_SECOND_SIGN_CH", APP.vars.currentChannel);
		APP.updateUI("MATH_CONST_CH", APP.vars.currentChannel);
	}

	APP.UI.update_trigger_menu = function() {
		APP.UI.update_color_bar();
	}

	APP.UI.update_save_menu = function() {
		APP.UI.update_color_bar();
	}

	APP.UI.close_menu = function(slideUp = true) {
		// Close current menu
		switch(APP.vars.currentMenu) {
			case Menu.INPUT_CHANNEL_1:
				$("#input-1-button").removeClass("active");
				if (slideUp) $("#INPUT_MENU").slideUp(300);
				else $("#INPUT_MENU").hide();
				break;
			case Menu.INPUT_CHANNEL_2:
				$("#input-2-button").removeClass("active");
				if (slideUp) $("#INPUT_MENU").slideUp(300);
				else $("#INPUT_MENU").hide();
				break;
			case Menu.INPUT_CHANNEL_3:
				$("#input-3-button").removeClass("active");
				if (slideUp) $("#INPUT_MENU").slideUp(300);
				else $("#INPUT_MENU").hide();
				break;
			case Menu.INPUT_CHANNEL_4:
				$("#input-4-button").removeClass("active");
				if (slideUp) $("#INPUT_MENU").slideUp(300);
				else $("#INPUT_MENU").hide();
				break;
			case Menu.OUTPUT_CHANNEL_1:
				$("#output-1-button").removeClass("active");
				if (slideUp) $("#OUTPUT_MENU").slideUp(300);
				else $("#OUTPUT_MENU").hide();
				break;
			case Menu.OUTPUT_CHANNEL_2:
				$("#output-2-button").removeClass("active");
				if (slideUp) $("#OUTPUT_MENU").slideUp(300);
				else $("#OUTPUT_MENU").hide();
				break;
			case Menu.MATH_CHANNEL_1:
				$("#math-1-button").removeClass("active");
				if (slideUp) $("#MATH_MENU").slideUp(300);
				else $("#MATH_MENU").hide();
				break;
			case Menu.MATH_CHANNEL_2:
				$("#math-2-button").removeClass("active");
				if (slideUp) $("#MATH_MENU").slideUp(300);
				else $("#MATH_MENU").hide();
				break;
			case Menu.TRIGGER:
				$("#trigger-button").removeClass("active");
				if (slideUp) $("#TRIGGER_MENU").slideUp(300);
				else $("#TRIGGER_MENU").hide();
				break;
			case Menu.SAVE:
				$("#save-button").removeClass("active");
				if (slideUp) $("#SAVE_MENU").slideUp(300);
				else $("#SAVE_MENU").hide();
				break;
			case Menu.CLOSE:
				$("#INPUT_MENU").hide();
				$("#OUTPUT_MENU").hide();
				$("#MATH_MENU").hide();
				$("#TRIGGER_MENU").hide();
				$("#SAVE_MENU").hide();
				break;
			default: return;
		};
		APP.vars.currentMenu = Menu.CLOSE;
		APP.UI.update_color_bar();
	}

	APP.UI.update_color_bar = function() {
		console.log("update color bar");
		switch(APP.vars.currentMenu) {
			case Menu.INPUT_CHANNEL_1:
				$("#color-bar").text("Input channel 1");
				$("#color-bar").css({"background-color": APP.getVal("OSC_SIGNAL_CH1_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.INPUT_CHANNEL_2:
				$("#color-bar").text("Input channel 2");
				$("#color-bar").css({"background-color": APP.getVal("OSC_SIGNAL_CH2_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.INPUT_CHANNEL_3:
				$("#color-bar").text("Input channel 3");
				$("#color-bar").css({"background-color": APP.getVal("OSC_SIGNAL_CH3_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.INPUT_CHANNEL_4:
				$("#color-bar").text("Input channel 4");
				$("#color-bar").css({"background-color": APP.getVal("OSC_SIGNAL_CH4_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.OUTPUT_CHANNEL_1:
				$("#color-bar").text("Ouput channel 1");
				$("#color-bar").css({"background-color": APP.getVal("GEN_SIGNAL_CH1_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.OUTPUT_CHANNEL_2:
				$("#color-bar").text("Ouput channel 2");
				$("#color-bar").css({"background-color": APP.getVal("GEN_SIGNAL_CH2_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.MATH_CHANNEL_1:
				$("#color-bar").text("Math channel 1");
				$("#color-bar").css({"background-color": APP.getVal("MATH_SIGNAL_CH1_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.MATH_CHANNEL_2:
				$("#color-bar").text("Math channel 2");
				$("#color-bar").css({"background-color": APP.getVal("MATH_SIGNAL_CH2_COLOR"), "color": "var(--text-white)"});
				break;
			case Menu.TRIGGER:
				$("#color-bar").text("Trigger menu");
				$("#color-bar").css({"background-color": "var(--header-color)", "color": "var(--text-white)"});
				break;
			case Menu.SAVE:
				$("#color-bar").text("Save menu");
				$("#color-bar").css({"background-color": "var(--header-color)", "color": "var(--text-white)"});
				break;
			case Menu.CLOSE:
				$("#color-bar").text("Select a menu");
				$("#color-bar").css({"background-color": "var(--right-section-color)", "color": "var(--text-grey)"});
				break;
			default: return;
		};
	}

	APP.UI.toggle_input_1_menu = function(event) {
		console.log("toggle_input_1_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.INPUT_CHANNEL_1) {
			APP.UI.close_menu(false);
			$("#input-1-button").addClass("active");
			$("#INPUT_MENU").slideDown(300);

			APP.vars.currentChannel = 1;
			APP.vars.currentMenu = Menu.INPUT_CHANNEL_1;
			APP.UI.update_osc_menu();

		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_input_2_menu = function(event) {
		console.log("toggle_input_2_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.INPUT_CHANNEL_2) {
			APP.UI.close_menu(false);
			$("#input-2-button").addClass("active");
			$("#INPUT_MENU").slideDown(300);
			APP.vars.currentChannel = 2;
			APP.vars.currentMenu = Menu.INPUT_CHANNEL_2;
			APP.UI.update_osc_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_input_3_menu = function(event) {
		console.log("toggle_input_3_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.INPUT_CHANNEL_3) {
			APP.UI.close_menu(false);
			$("#input-3-button").addClass("active");
			$("#INPUT_MENU").slideDown(300);
			APP.vars.currentChannel = 3;
			APP.vars.currentMenu = Menu.INPUT_CHANNEL_3;
			APP.UI.update_osc_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_input_4_menu = function(event) {
		console.log("toggle_input_4_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.INPUT_CHANNEL_4) {
			APP.UI.close_menu(false);
			$("#input-4-button").addClass("active");
			$("#INPUT_MENU").slideDown(300);
			APP.vars.currentChannel = 4;
			APP.vars.currentMenu = Menu.INPUT_CHANNEL_4;
			APP.UI.update_osc_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_output_1_menu = function(event) {
		console.log("toggle_output_1_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.OUTPUT_CHANNEL_1) {
			APP.UI.close_menu(false);
			$("#output-1-button").addClass("active");
			$("#OUTPUT_MENU").slideDown(300);
			APP.vars.currentChannel = 1;
			APP.vars.currentMenu = Menu.OUTPUT_CHANNEL_1;
			APP.UI.update_gen_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_output_2_menu = function(event) {
		console.log("toggle_output_2_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.OUTPUT_CHANNEL_2) {
			APP.UI.close_menu(false);
			$("#output-2-button").addClass("active");
			$("#OUTPUT_MENU").slideDown(300);
			APP.vars.currentChannel = 2;
			APP.vars.currentMenu = Menu.OUTPUT_CHANNEL_2;
			APP.UI.update_gen_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_math_1_menu = function(event) {
		console.log("toggle_math_1_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.MATH_CHANNEL_1) {
			APP.UI.close_menu(false);
			$("#math-1-button").addClass("active");
			$("#MATH_MENU").slideDown(300);
			APP.vars.currentChannel = 1;
			APP.vars.currentMenu = Menu.MATH_CHANNEL_1;
			APP.UI.update_math_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_math_2_menu = function(event) {
		console.log("toggle_math_2_menu");
		var old = APP.vars.currentMenu;
		if (old !== Menu.MATH_CHANNEL_2) {
			APP.UI.close_menu(false);
			$("#math-2-button").addClass("active");
			$("#MATH_MENU").slideDown(300);
			APP.vars.currentChannel = 2;
			APP.vars.currentMenu = Menu.MATH_CHANNEL_2;
			APP.UI.update_math_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_trigger_menu = function(event) {
		var old = APP.vars.currentMenu;
		if (old !== Menu.TRIGGER) {
			APP.UI.close_menu(false);
			APP.UI.update_color_bar();
			$("#trigger-button").addClass("active");
			$("#TRIGGER_MENU").slideDown(300);
			APP.vars.currentMenu = Menu.TRIGGER;
			APP.UI.update_trigger_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.toggle_save_menu = function(event) {
		var old = APP.vars.currentMenu;
		if (old !== Menu.SAVE) {
			APP.UI.close_menu(false);
			APP.UI.update_color_bar();
			$("#save-button").addClass("active");
			$("#SAVE_MENU").slideDown(300);
			APP.vars.currentMenu = Menu.SAVE;
			APP.UI.update_save_menu();
		} else {
			APP.UI.close_menu(true);
		}
	}

	APP.UI.update_plot = function() {
		APP.plot.getAxes().xaxis.options.min = 0;
		APP.plot.getAxes().xaxis.options.max = APP.signals.length_display;
		APP.plot.getAxes().yaxis.options.min = APP.getVal("PLOT_Y_MIN");
		APP.plot.getAxes().yaxis.options.max = APP.getVal("PLOT_Y_MAX");
		APP.update_x_zoom();

		APP.plot.resize();
		APP.plot.setupGrid();
		APP.plot.draw();
	}

	APP.UI.change_decrease_x = function() {
		// Decrease the max value of the x-axis
		console.log("decrease x-axis");
		var plot_x_max = APP.getVal("PLOT_X_MAX");
		plot_x_max -= APP.getVal("JOYSTIC_PRECISION");
		if (plot_x_max > 1e-9)
			APP.setVal("PLOT_X_MAX", plot_x_max);
		APP.UI.update_plot();
	}
	
	APP.UI.change_increase_x = function() {
		// Increase the max value of the x-axis
		console.log("increase x-axis");
		var plot_x_max = APP.getVal("PLOT_X_MAX");
		plot_x_max += APP.getVal("JOYSTIC_PRECISION");
		APP.setVal("PLOT_X_MAX", plot_x_max);
		APP.UI.update_plot();
	}
	
	APP.UI.change_decrease_y = function() {
		// Decrease the max value of the y-axis (crête à crête)
		console.log("decrease y-axis");
		var plot_y_max = APP.getVal("PLOT_Y_MAX");
		plot_y_max -= APP.getVal("JOYSTIC_PRECISION");
		if (plot_y_max > 1e-9)
			APP.setVal("PLOT_Y_MAX", plot_x_max);
		var plot_y_min = APP.getVal("PLOT_Y_MAX");
		if (plot_y_min < -1e-9)
			plot_y_min += APP.getVal("JOYSTIC_PRECISION");
		APP.setVal("PLOT_Y_MIN", plot_x_max);
		APP.UI.update_plot();
	}
	
	APP.UI.change_increase_y = function() {
		// Increase the max value of the y-axis (crête à crête)
		console.log("increase y-axis");
		var plot_y_max = APP.getVal("PLOT_Y_MAX");
		plot_y_max += APP.getVal("JOYSTIC_PRECISION");
		APP.setVal("PLOT_Y_MAX", plot_x_max);
		var plot_y_min = APP.getVal("PLOT_Y_MAX");
		plot_y_min -= APP.getVal("JOYSTIC_PRECISION");
		APP.setVal("PLOT_Y_MIN", plot_x_max);
		APP.UI.update_plot();
	}
	
	APP.UI.change_precision_up = function() {
		// Cycle through different precision levels
		var precision = APP.getVal("JOYSTIC_PRECISION");
		precision *= 10;
		if (APP.getVal("JOYSTIC_PRECISION") > 10) precision = 1e-9;
		$("#precision").html(APP.formatNumber(precision, 0));
		APP.setVal("JOYSTIC_PRECISION", precision)
	}

	APP.UI.change_precision_down = function() {
		// Cycle through different precision levels
		var precision = APP.getVal("JOYSTIC_PRECISION");
		precision /= 10;
		if (APP.getVal("JOYSTIC_PRECISION") < 1e-9) precision = 10;
		$("#precision").html(APP.formatNumber(precision, 0));
		APP.setVal("JOYSTIC_PRECISION", precision)
	}



	//Create callback
	APP.UI.changeCallbacks = {}

	APP.UI.changeCallbacks["OSC_SHOW_CH"]       = APP.UI.change_osc_show;
	APP.UI.changeCallbacks["OSC_COUPLING_CH"]   = APP.UI.change_couplig;
	APP.UI.changeCallbacks["OSC_INVERT_CH"]     = APP.UI.change_osc_invert;
	APP.UI.changeCallbacks["OSC_PROBE_CH"]       = APP.UI.change_osc_probe;
	APP.UI.changeCallbacks["OSC_GAIN_CH"]       = APP.UI.change_osc_gain;

	APP.UI.changeCallbacks["GEN_ENABLE_CH"]     = APP.UI.change_gen_enable;
	APP.UI.changeCallbacks["GEN_SHOW_CH"]       = APP.UI.change_gen_show;
	APP.UI.changeCallbacks["GEN_FREQUENCY_CH"]  = APP.UI.change_gen_frequency;
	APP.UI.changeCallbacks["GEN_AMPLITUDE_CH"]  = APP.UI.change_gen_amplitude;
	APP.UI.changeCallbacks["GEN_OFFSET_CH"]     = APP.UI.change_gen_offset;
	APP.UI.changeCallbacks["GEN_WAVEFORM_CH"]   = APP.UI.change_gen_waveform;
	APP.UI.changeCallbacks["GEN_MODE_CH"]       = APP.UI.change_gen_mode; 
	APP.UI.changeCallbacks["GEN_DUTY_CYCLE_CH"] = APP.UI.change_gen_duty_cycle;
	APP.UI.changeCallbacks["GEN_BURST_COUNT_CH"]= APP.UI.change_gen_burst_count;
	APP.UI.changeCallbacks["GEN_BURST_PER_CH"]  = APP.UI.change_gen_burst_per;
	APP.UI.changeCallbacks["GEN_BURST_REP_CH"]  = APP.UI.change_gen_burst_rep;
	APP.UI.changeCallbacks["GEN_IMPEDANCE_CH"]  = APP.UI.change_gen_impedance;
	APP.UI.changeCallbacks["GEN_RISE_TIME_CH"]  = APP.UI.change_gen_rise_time;
	APP.UI.changeCallbacks["GEN_FALL_TIME_CH"]  = APP.UI.change_gen_fall_time;

	APP.UI.clickCallbacks = {}
	APP.UI.clickCallbacks["APP_RUN"]           = APP.UI.change_app_run;

	APP.UI.clickCallbacks["input-1-button"]    = APP.UI.toggle_input_1_menu;
	APP.UI.clickCallbacks["input-2-button"]    = APP.UI.toggle_input_2_menu;
	APP.UI.clickCallbacks["input-3-button"]    = APP.UI.toggle_input_3_menu;
	APP.UI.clickCallbacks["input-4-button"]    = APP.UI.toggle_input_4_menu;
	APP.UI.clickCallbacks["output-1-button"]   = APP.UI.toggle_output_1_menu;
	APP.UI.clickCallbacks["output-2-button"]   = APP.UI.toggle_output_2_menu;
	APP.UI.clickCallbacks["math-1-button"]     = APP.UI.toggle_math_1_menu;
	APP.UI.clickCallbacks["math-2-button"]     = APP.UI.toggle_math_2_menu;
	APP.UI.clickCallbacks["trigger-button"]    = APP.UI.toggle_trigger_menu;
	APP.UI.clickCallbacks["save-button"]       = APP.UI.toggle_save_menu;

	APP.UI.clickCallbacks["decrease-x"]  = APP.UI.change_decrease_x;
	APP.UI.clickCallbacks["increase-x"]  = APP.UI.change_increase_x;
	APP.UI.clickCallbacks["decrease-y"]  = APP.UI.change_decrease_y;
	APP.UI.clickCallbacks["increase-y"]  = APP.UI.change_increase_y;
	APP.UI.clickCallbacks["precision"]   = APP.UI.change_precision_up;
	APP.UI.clickCallbacks["precision-up"]   = APP.UI.change_precision_up;
	APP.UI.clickCallbacks["precision-down"]   = APP.UI.change_precision_down;

}(window.APP = window.APP || {}, jQuery));
