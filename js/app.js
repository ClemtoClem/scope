(function() {
	var originalAddClassMethod = jQuery.fn.addClass;
	var originalRemoveClassMethod = jQuery.fn.removeClass;
	$.fn.addClass = function(clss) {
		var result = originalAddClassMethod.apply(this, arguments);
		$(this).trigger('activeChanged', 'add');
		return result;
	};
	$.fn.removeClass = function(clss) {
		var result = originalRemoveClassMethod.apply(this, arguments);
		$(this).trigger('activeChanged', 'remove');
		return result;
	}
})();

// Frozen enum - Immutable

const Waveform = Object.freeze({
	SINE		:0,	//!< Wave form sine
	SQUARE		:1,	//!< Wave form square
	TRIANGLE	:2,	//!< Wave form triangle
	RAMP_UP		:3,	//!< Wave form sawtooth (/|)
	RAMP_DOWN	:4,	//!< Wave form reversed sawtooth (|\)
	DC			:5,	//!< Wave form dc
	PWM			:6,	//!< Wave form pwm
	//ARBITRARY	:7,	//!< Use defined wave form
	DC_NEG		:8,	//!< Wave form negative dc
	//SWEEP		:9,	//!< Wave form sweep
});

const GenMode = Object.freeze({
	CONTINUOUS	:0,	//!< Continuous signal generation
	BURST		:1,	//!< Signal is generated N times, wher N is defined with rp_GenBurstCount method
	STREAM		:2	//!< User can continuously write data to buffer
});

const Menu = Object.freeze({
	CLOSE				:0,
	INPUT_CHANNEL_1		:1,
	INPUT_CHANNEL_2		:2,
	INPUT_CHANNEL_3		:3,
	INPUT_CHANNEL_4		:4,
	OUTPUT_CHANNEL_1	:5,
	OUTPUT_CHANNEL_2	:6,
	MATH_CHANNEL_1	    :7,
	MATH_CHANNEL_2  	:8,
	TRIGGER				:9,
	SAVE                :10
});



(function(APP, $, unused) {

	// Params cache
	APP.params = {
		shared: {},
		local: {
			/* couleur des signaux sur le plot */
			'OSC_SIGNAL_CH1_COLOR': {value: "#FF6600"},
			'OSC_SIGNAL_CH2_COLOR': {value: "#11942B"},
			'OSC_SIGNAL_CH3_COLOR': {value: "#660099"},
			'OSC_SIGNAL_CH4_COLOR': {value: "#00B7DF"},
			'GEN_SIGNAL_CH1_COLOR': {value: "#8E28C1"},
			'GEN_SIGNAL_CH2_COLOR': {value: "#C60F11"},
			'MATH_SIGNAL_CH1_COLOR': {value: "#660033"},
			'MATH_SIGNAL_CH2_COLOR': {value: "#763B10"},

			"JOYSTIC_PRECISION": {value: 1},
			"PLOT_X_MIN": {value: 0, min:1e-9, max:600},
			"PLOT_X_MAX": {value: 1, min:1e-9, max:600},
			"PLOT_Y_MIN": {value:-5, min:-20, max:1e-6},
			"PLOT_Y_MAX": {value: 5, min:1e-6, max:20}
		},
		callback: {}
	};

	const length_display_signal = 1024;
	APP.signals = {
		length_display: length_display_signal,
		
		OSC_SIGNAL_CH1: Array(length_display_signal).fill(0),
		OSC_SIGNAL_CH2: Array(length_display_signal).fill(0),
		OSC_SIGNAL_CH3: Array(length_display_signal).fill(0),
		OSC_SIGNAL_CH4: Array(length_display_signal).fill(0),

		GEN_SIGNAL_CH1: Array(length_display_signal).fill(0),
		GEN_SIGNAL_CH2: Array(length_display_signal).fill(0),

		MATH_SIGNAL_CH1: Array(length_display_signal).fill(0),
		MATH_SIGNAL_CH2: Array(length_display_signal).fill(0),
	};

	// Other global variables
	APP.ws = null;
	APP.plot = {};

	APP.parameterStack = [];
	APP.signalStack = [];
	// Parameters cache
	APP.parametersCache = {};

	// App configuration
	APP.config = {};
	APP.config.app_id = 'scope';
	APP.config.server_ip = ''; // Leave empty on production, it is used for testing only
	APP.config.run_app_url = (APP.config.server_ip.length ? 'http://' + APP.config.server_ip : '') + '/bazaar?start=' + APP.config.app_id;
	APP.config.stop_app_url = (APP.config.server_ip.length ? 'http://' + APP.config.server_ip : '') + '/bazaar?stop=' + APP.config.app_id;
	APP.config.socket_url = 'ws://' + (APP.config.server_ip.length ? APP.config.server_ip : window.location.hostname) + '/wss'; // WebSocket server URI
	APP.config.previousPageUrl = '/';

	// App global vars
	APP.vars = {};
	APP.vars.socket_opened = false;
	APP.vars.processing = false;
	APP.vars.parametersCache_isEmpty = true;
	APP.vars.currentMenu = Menu.CLOSE;
	APP.vars.currentChannel = 1;
	APP.vars.running = false;
	APP.vars.unexpectedClose = true;

		// Graph state
	APP.vars.editing = false;
	APP.vars.time_dragging = false;
	APP.vars.trig_dragging = false;
	APP.vars.cursor_dragging = false;
	APP.vars.cursor_dragging_measure =false;
	APP.vars.simulated_drag =false;
	APP.vars.mouseover = false;
	APP.vars.resized = false;
	APP.vars.sel_sig_name = 'ch1';
	APP.vars.fine = false;
	APP.vars.graph_grid_height = null;
	APP.vars.graph_grid_width = null;
	APP.vars.calib = 0;


	APP.startApp = function() {
		$.get(APP.config.run_app_url)
			.done(function(dresult) {
				if (dresult.status == 'OK') {
					try {
						APP.connectWebSocket();
						console.log("Load manager");
					} catch (e) {
						APP.startApp();
					}
				} else if (dresult.status == 'ERROR') {
					console.log(dresult.reason ? dresult.reason : 'Could not start the application (ERR1)');
					APP.startApp();
				} else {
					console.log('Could not start the application (ERR2)');
					APP.startApp();
				}
			})
			.fail(function() {
				console.log('Could not start the application (ERR3)');
				APP.startApp();
			});
	};



	// Creates a WebSocket connection with the web server
	APP.connectWebSocket = function() {
		if (window.WebSocket) {
			APP.ws = new WebSocket(APP.config.socket_url);
			APP.ws.binaryType = "arraybuffer";
		} else if (window.MozWebSocket) {
			APP.ws = new MozWebSocket(APP.config.socket_url);
			APP.ws.binaryType = "arraybuffer";
		} else {
			console.log('Browser does not support WebSocket');
		}

		// Define WebSocket event listeners
		if (APP.ws) {
			APP.ws.onopen = function() {
				console.log('Socket opened');

				APP.vars.socket_opened = true;
				APP.sendParameters(true);
				APP.vars.unexpectedClose = true;

			};

			APP.ws.onclose = function() {
				console.log('Socket closed');

				APP.vars.socket_opened = false;
				if (APP.vars.unexpectedClose === true)
					setTimeout(APP.startApp(), 1000);
			};

			APP.ws.onerror = function(ev) {
				console.log('Websocket error: ', ev);

				if (!APP.vars.socket_opened)
					APP.startApp();
			};

			APP.ws.onmessage = function(ev) {
				try {
					var data = new Uint8Array(ev.data);
					var inflate = pako.inflate(data);
					var text = String.fromCharCode.apply(null, new Uint8Array(inflate));
					//console.log(text);
					var receive = JSON.parse(text);

					// Recieving parameters
					if (receive.parameters) {
						APP.parameterStack.push(receive.parameters);
					}

					// Recieve signals
					if (receive.signals) {
						//g_PacketsRecv++;
						APP.signalStack.push(receive.signals);
					}

				} catch (e) {
					console.log(e);
				} finally {}
			};
		}
	};

	// For Firefox
	function fireEvent(obj, evt) {
		var fireOnThis = obj;
		if (document.createEvent) {
			var evObj = document.createEvent('MouseEvents');
			evObj.initEvent(evt, true, false);
			fireOnThis.dispatchEvent(evObj);

		} else if (document.createEventObject) {
			var evObj = document.createEventObject();
			fireOnThis.fireEvent('on' + evt, evObj);
		}
	}

	// Sends to server parameters
	APP.sendParameters = function(force = false) {
		if (!APP.vars.socket_opened) {
			console.log('ERROR: Cannot save changes, socket not opened');
			return false;
		}
		if ((!APP.vars.parametersCache_isEmpty || force) && !APP.vars.processing) {
			APP.vars.processing = true;

			APP.parametersCache["in_command"] = { value: "send_all_params" };
			APP.ws.send(JSON.stringify({ parameters: APP.parametersCache }));
			APP.parametersCache = {};

			APP.vars.parametersCache_isEmpty = true;
			APP.vars.processing = false;
		}
		return true;
	};


	/* -------------------------------------------------------------------- */
	
	
	// Lire la valeur d'un paramètre partagé ou local
	APP.getVal = function(param_name) {
		if (param_name in APP.params.shared) {
			return APP.params.shared[param_name].value;
		} else if (param_name in APP.params.local) {
			//console.log('get local param value:', param_name, APP.params.local[param_name].value);
			return APP.params.local[param_name].value;
		} else
			return 0;
	}

	// Modifier la valeur d'un paramètre partagé ou local
	APP.setVal = function(param_name, value, send = false) {
		if (param_name in APP.params.shared) {
			console.log("set value of shared param ", param_name);
			if (typeof(APP.params.shared[param_name].value) !== "string") {
				if (value > APP.params.shared[param_name].max) value = APP.params.shared[param_name].max;
				if (value < APP.params.shared[param_name].min) value = APP.params.shared[param_name].min;
				APP.params.shared[param_name].value = value;
			} else {
				value = toString(value);
				APP.params.shared[param_name].value = value;
			}

			if (send === true) {
				console.log("app parameters in cache");
				APP.parametersCache[param_name] = { "value": value };
				APP.vars.parametersCache_isEmpty = false;
			}
		} else if (param_name in APP.params.local) {
			console.log("set value of local param ", param_name);
			if (typeof(APP.params.local[param_name].value) !== "string") {
				if (value > APP.params.local[param_name].max) value = APP.params.local[param_name].max;
				if (value < APP.params.local[param_name].min) value = APP.params.local[param_name].min;
				APP.params.local[param_name].value = value;
			} else {
				value = toString(value);
				APP.params.local[param_name].value = value;
			}
		}
		return value;
	};

	// Mettre à jour un paramètre à partir de la valeur d'un élément js du même nom
	APP.updateVal = function(param_name, channel = "", proprety = "") {
		var val;
		if (proprety !== "" && typeof(proprety) === "string") {
			val = APP.setVal(param_name + channel, $("#" + param_name).prop(proprety), true);
			$("#" + param_name).prop(proprety, val);
		} else {
			val = APP.setVal(param_name + channel, $("#" + param_name).val(), true);
			$("#" + param_name).val(val);
		}
		console.log("Update value: "+param_name + channel+" =", val);
		return val;
	}

	APP.updateBoolVal = function(param_name, channel = "", proprety = "") {
		var val;
		if (proprety !== "" && typeof(proprety) === "string") {
			var state = false;
			if (typeof($("#" + param_name).prop(proprety)) === 'string') {
				if ($("#" + param_name).prop(proprety) === 'true') state = true;
			} else {
				state = Boolean($("#" + param_name).prop(proprety));
			}
			val = APP.setVal(param_name + channel, state, true);
			$("#" + param_name).prop(proprety, val);
		} else {
			var state = false;
			if (typeof($("#" + param_name).val()) === 'string') {
				if ($("#" + param_name).val() === 'true') state = true;
			} else {
				state = Boolean($("#" + param_name).val());
			}
			val = APP.setVal(param_name + channel, state, true);
			$("#" + param_name).val(val);
		}
		return val;
	}

	APP.updateIntVal = function(param_name, channel = "", proprety = "") {
		var val;
		if (proprety !== "" && typeof(proprety) === "string") {
			val = APP.setVal(param_name + channel, parseInt($("#" + param_name).prop(proprety)), true);
			$("#" + param_name).prop(proprety, val);
		} else {
			val = APP.setVal(param_name + channel, parseInt($("#" + param_name).val()), true);
			$("#" + param_name).val(val);
		}
		console.log("Update integer value: "+param_name + channel+" =", val);
		return val;
	}

	APP.updateFloatVal = function(param_name, channel = "", proprety = "") {
		var val;
		if (proprety !== "" && typeof(proprety) === "string") {
			val = APP.setVal(param_name + channel, parseFloat($("#" + param_name).prop(proprety)), true);
			$("#" + param_name).prop(proprety, val);
		} else {
			val = APP.setVal(param_name + channel, parseFloat($("#" + param_name).val()), true);
			$("#" + param_name).val(val);
		}
		console.log("Update float value: "+param_name + channel+" =", val);
		return val;
	}

	// Mettre à jour la valeur d'un élément js à partir de un paramètre du même nom
	APP.updateUI = function(param_name, channel = "", proprety = "") {
		var val;
		if (proprety !== "" && typeof(proprety) === "string") {
			val = APP.getVal(param_name + channel);
			$("#" + param_name).prop(proprety, val);
		} else {
			val = APP.getVal(param_name + channel);
			$("#" + param_name).val(val);
		}
		console.log("Update UI: "+param_name + channel+" =", val);
		return val;
	}


	/* -------------------------------------------------------------------- */


	// Processes newly received data for signals
	APP.processSignals = function(new_signals) {
		var newDatas = [];

		const skip_points = APP.signals.skip_points;
		for (sig_name in new_signals) {
			//var id_channel = sig_name.match(/\d+$/)[0];

			if (!(sig_name in APP.signals)) {
				APP.signals[sig_name] = Array(APP.signals.length_display).fill(0);
			}
			if (new_signals[sig_name].size !== 0) {
				var pointsXY = [];
				var sig_size = Math.floor(new_signals[sig_name].size/skip_points);
				var ind_offset = APP.signals.length_display - sig_size;
				for (i = 0; i < ind_offset; i++) {
					APP.signals[sig_name][i] = APP.signals[sig_name][i+sig_size];
					pointsXY.push([i, APP.signals[sig_name][i]]);
				}

				for (var i = 0; i < sig_size; i ++) {
					APP.signals[sig_name][i+ind_offset] = new_signals[sig_name].value[Math.round(i*skip_points)];
					pointsXY.push([i+ind_offset, APP.signals[sig_name][i+ind_offset]]);
				}

				newDatas.push({
					data: pointsXY,
					color: APP.getVal(sig_name+"_COLOR")
				});
			}
		}

		// Update graph
		APP.plot.setData(newDatas);
		APP.plot.resize();
		APP.plot.setupGrid();
		APP.plot.draw();
	}

	//Handler
	var signalsHandler = function() {
		if (!APP.vars.socket_opened || !APP.vars.running) return;

		while (APP.signalStack.length > 0) {
			APP.processSignals(APP.signalStack[0]);
			APP.signalStack.splice(0, 1);
		}
	}

	APP.processParameters = function(new_params) {
		
		var old_params = $.extend(true, {}, APP.params.shared);

		var send_all_params = Object.keys(new_params).indexOf('send_all_params') != -1;
		
		if ('APP_RUN' in new_params) {
			APP.vars.running = new_params['APP_RUN'].value;
		}
		
		for (var param_name in new_params) {
			APP.params.shared[param_name] = new_params[param_name];
			if (APP.params.callback[param_name] !== undefined)
				APP.params.callback[param_name](new_params);
		}
		
		// On attend de recevoir les premiers paramètres de l'application C avant de laisser
		// la main à l'utilisateur pour changer les paramètres
		if ($("body").hasClass("loaded") === false) {
			console.log("Recept init parms ", new_params);
			$("body").addClass("loaded");

			// on masque l"icon de chargement
			var $element = $("#loader-wrapper");
			if ($element.length == 0){
				$element.remove();
			}
		}
	};

	var parametersHandler = function() {
		if (!APP.vars.socket_opened) return;

		if (APP.parameterStack.length > 0) {
			APP.processParameters(APP.parameterStack[0]);
			APP.parameterStack.splice(0, 1);
		}

		APP.sendParameters();
	}

	//Set handlers timers
	setInterval(signalsHandler, 10);
	setInterval(parametersHandler, 1);



	/* -------------------------------------------------------------------- */

	APP.update_x_zoom = function() {
		if ('OSC_TIME_SCALE' in APP.params.shared) {
			var time_scale = APP.getVal("OSC_TIME_SCALE");
			var view_part = APP.getVal("OSC_VIEW_PART");
			var xmax = APP.getVal("PLOT_X_MAX");

			APP.signals.skip_points = xmax/(time_scale*view_part);
			console.log("skip points : ", APP.signals.skip_points);
			console.log("add sign size : ", Math.floor(1024/APP.signals.skip_points));
		}
	}


	/* -------------------------------------------------------------------- */

	var getMenuInput = function(ch) {
		var menu = -1;
		switch (ch) {
			case 1: menu = Menu.INPUT_CHANNEL_1; break;
			case 2: menu = Menu.INPUT_CHANNEL_2; break;
			case 3: menu = Menu.INPUT_CHANNEL_3; break;
			case 4: menu = Menu.INPUT_CHANNEL_4; break;
		}
		return menu;
	}
	var getMenuOutput = function(ch) {
		var menu = -1;
		switch (ch) {
			case 1: menu = Menu.OUTPUT_CHANNEL_1; break;
			case 2: menu = Menu.OUTPUT_CHANNEL_2; break;
		}
		return menu;
	}
	var getMenuMath = function(ch) {
		var menu = -1;
		switch (ch) {
			case 1: menu = Menu.MATH_CHANNEL_1; break;
			case 2: menu = Menu.MATH_CHANNEL_2; break;
		}
		return menu;
	}


	APP.process_app_run = function(new_params) {
		$("#APP_RUN").text(new_params['APP_RUN'].value? "RUN":"STOP");
		$("#APP_RUN").css("background-color", new_params['APP_RUN'].value? "green":"red");
	}

	APP.process_adc_count = function(new_params) {
		if (new_params['ADC_COUNT'].value == 4) {
			console.log("Show input button 3 & 4");
			// Afficher les boutons pour les channels 3 et 4
			$("#input-3-button").show();
			$("#input-4-button").show();
			// Afficher les options avec les valeurs 2 et 3
			$("#MATH_FIRST_SIGN_CH option[value='2']").show();
			$("#MATH_FIRST_SIGN_CH option[value='3']").show();
			$("#MATH_SECOND_SIGN_CH option[value='2']").show();
			$("#MATH_SECOND_SIGN_CH option[value='3']").show();
		} else if (new_params['ADC_COUNT'].value == 2) {
			console.log("Hide input button 3 & 4");
			// Masquer les boutons pour les channels 3 et 4
			$("#input-3-button").hide();
			$("#input-4-button").hide();
			// Masquer les options avec les valeurs 2 et 3
			$("#MATH_FIRST_SIGN_CH option[value='2']").hide();
			$("#MATH_FIRST_SIGN_CH option[value='3']").hide();
			$("#MATH_SECOND_SIGN_CH option[value='2']").hide();
			$("#MATH_SECOND_SIGN_CH option[value='3']").hide();
		}
	}

	APP.process_sample_rate = function(new_params) {
		var adc_rate;
		if (new_params['ADC_RATE'] != undefined)
			adc_rate = new_params['ADC_RATE'].value;
		else
			adc_rate = APP.getVal("ADC_RATE");

		var curRate = adc_rate / new_params['OSC_SAMPLE_RATE'].value;
		var val = curRate;
		var suf = ""
		if (curRate > 1000000) {
			suf = "M"
			val  = (curRate / 1000000).toFixed(3);;

		} else if (curRate > 1000) {
			suf = "k";
			val  = (curRate / 1000).toFixed(3);;
		}
		$("#OSC_SAMPLE_RATE").text(val + " " + suf+ 'S/s');
	}

	APP.process_view_part = function(new_params) {
		$("#OSC_VIEW_PART").val(new_params['OSC_VIEW_PART'].value);
		console.log("view part ", new_params['OSC_VIEW_PART'].value );
		APP.update_x_zoom();
	}

	APP.process_time_scale = function(new_params) {
		$("#OSC_TIME_SCALE").val(new_params['OSC_TIME_SCALE'].value);
		console.log("time scale ", new_params['OSC_TIME_SCALE'].value );
		APP.update_x_zoom();
	}

	APP.process_osc_show_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuInput(ch))
			$("#OSC_SHOW_CH").prop("checked", new_params['OSC_SHOW_CH'+ch].value);
	}

	APP.process_osc_couplig_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuInput(ch))
			$("#OSC_COUPLING_CH").val(new_params['OSC_COUPLING_CH1'].value);
	}

	APP.process_osc_invert_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuInput(ch))
			$("#OSC_INVERT_CH").prop("checked", new_params['OSC_INVERT_CH'+ch].value);
	}

	APP.process_osc_probe_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuInput(ch))
			$("#OSC_PROBE_CH").prop("checked", new_params['OSC_PROBE_CH'+ch].value);
	}
	
	APP.process_osc_gain_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuInput(ch))
			$("#OSC_GAIN_CH").prop("checked", new_params['OSC_GAIN_CH'+ch].value);
	}

	APP.process_gen_enable_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_ENABLE_CH").prop("checked", new_params['GEN_ENABLE_CH'+ch].value);
	}

	APP.process_gen_show_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_SHOW_CH").prop("checked", new_params['GEN_SHOW_CH'+ch].value);
	}

	APP.process_gen_amplitude_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_AMPLITUDE_CH").val(new_params['GEN_AMPLITUDE_CH'+ch].value);
	}

	APP.process_gen_frequency_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_FREQUENCY_CH").val(new_params['GEN_FREQUENCY_CH'+ch].value);
	}

	APP.process_gen_offset_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_OFFSET_CH").val(new_params['GEN_OFFSET_CH'+ch].value);
	}

	APP.process_gen_waveform_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch)) {
			var val = new_params['GEN_WAVEFORM_CH'+ch].value;
			$("#GEN_WAVEFORM_CH").val(val);
			APP.UI.update_gen_waveform_section(val);
		}
	}

	APP.process_gen_mode_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch)) {
			var val = new_params['GEN_MODE_CH'+ch].value;
			$("#GEN_MODE_CH").val(val);
			APP.UI.update_gen_mode_section(val);
		}
	}

	APP.process_gen_duty_cycle_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_DUTY_CYCLE_CH").val(new_params['GEN_DUTY_CYCLE_CH'+ch].value);
	}

	APP.process_gen_burst_count_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_BURST_COUNT_CH").val(new_params['GEN_BURST_COUNT_CH'+ch].value);
	}

	APP.process_gen_burst_rep_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_BURST_REP_CH").val(new_params['GEN_BURST_REP_CH'+ch].value);
	}

	APP.process_gen_burst_per_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_BURST_PER_CH").val(new_params['GEN_BURST_PER_CH'+ch].value);
	};

	APP.process_gen_impedance_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_IMPEDANCE_CH"+ch).val(new_params['GEN_IMPEDANCE_CH'+ch].value);
	};

	APP.process_gen_rise_time_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_RISE_TIME_CH"+ch).val(new_params['GEN_RISE_TIME_CH'+ch].value);
	};

	APP.process_gen_fall_time_ch = function(ch, new_params) {
		if (APP.vars.currentMenu === getMenuOutput(ch))
			$("#GEN_FALL_TIME_CH"+ch).val(new_params['GEN_FALL_TIME_CH'+ch].value);
	};

	APP.params.callback["APP_RUN"]   = APP.process_app_run;
	APP.params.callback["ADC_COUNT"] = APP.process_adc_count;

	APP.params.callback["OSC_VIEW_PART"]  = APP.process_view_part;
	APP.params.callback["OSC_TIME_SCALE"] = APP.process_time_scale;

	for (var i = 1; i<=4; i++) {
		APP.params.callback["OSC_SHOW_CH"+i]	 = function(new_params) { APP.process_osc_show_ch(i, new_params); };
		APP.params.callback["OSC_COUPLING_CH"+i] = function(new_params) { APP.process_osc_couplig_ch(i, new_params); };
		APP.params.callback["OSC_INVERT_CH"+i] 	 = function(new_params) { APP.process_osc_invert_ch(i, new_params); };
		APP.params.callback["OSC_PROBE_CH"+i] 	 = function(new_params) { APP.process_osc_probe_ch(i, new_params); };
		APP.params.callback["OSC_GAIN_CH"+i] 	 = function(new_params) { APP.process_osc_gain_ch(i, new_params); };
	}

	for (var i = 1; i<=2; i++) {
		APP.params.callback["GEN_ENABLE_CH"+i] 	  = function(new_params) { APP.process_gen_enable_ch(i, new_params); }
		APP.params.callback["GEN_SHOW_CH"+i]      = function(new_params) { APP.process_gen_show_ch(i, new_params); }
		APP.params.callback["GEN_AMPLITUDE_CH"+i] = function(new_params) { APP.process_gen_amplitude_ch(i, new_params); }
		APP.params.callback["GEN_FREQUENCY_CH"+i] = function(new_params) { APP.process_gen_frequency_ch(i, new_params); }
		APP.params.callback["GEN_OFFSET_CH"+i]    = function(new_params) { APP.process_gen_offset_ch(i, new_params); }
		APP.params.callback["GEN_WAVEFORM_CH"+i]  = function(new_params) { APP.process_gen_waveform_ch(i, new_params); }
		APP.params.callback["GEN_MODE_CH"+i] 	  = function(new_params) { APP.process_gen_mode_ch(i, new_params); }
		APP.params.callback["GEN_DUTY_CYCLE_CH"+i]  = function(new_params) { APP.process_gen_duty_cycle_ch(i, new_params); }
		APP.params.callback["GEN_BURST_COUNT_CH"+i] = function(new_params) { APP.process_gen_burst_count_ch(i, new_params); }
		APP.params.callback["GEN_BURST_REP_CH"+i] = function(new_params) { APP.process_gen_burst_rep_ch(i, new_params); }
		APP.params.callback["GEN_BURST_PER_CH"+i] = function(new_params) { APP.process_gen_burst_per_ch(i, new_params); }
		APP.params.callback["GEN_IMPEDANCE_CH"+i] = function(new_params) { APP.process_gen_impedance_ch(i, new_params); }
		APP.params.callback["GEN_RISE_TIME_CH"+i] = function(new_params) { APP.process_gen_rise_time_ch(i, new_params); }
		APP.params.callback["GEN_FALL_TIME_CH"+i] = function(new_params) { APP.process_gen_fall_time_ch(i, new_params); }
	}





	// Fonction pour formater les valeurs
	APP.formatNumber = function(val, fixed = 2) {
		if (val === 0) return "0 ";
		var result = (val < 0) ? "-" : "";
		var abs_val = Math.abs(val);
		if (abs_val < 1e-6) {
			result += (abs_val * 1e9).toFixed(fixed) + " n";
		} else if (abs_val < 1e-3) {
			result += (abs_val * 1e6).toFixed(fixed) + " µ";
		} else if (abs_val < 1) {
			result += (abs_val * 1e3).toFixed(fixed) + " m";
		} else {
			result += abs_val.toFixed(fixed) + " ";
		}
		return result;
	}

}(window.APP = window.APP || {}, jQuery));


// Page onload event handler
$(function() {

	var reloaded = $.cookie("SM_forced_reload");
	if (reloaded == undefined || reloaded == "false") {
		$.cookie("SM_forced_reload", "true");
		window.location.reload(true);
	}

	// Init plot
	APP.plot = $.plot($("#placeholder"), [], { 
		series: {
			shadowSize: 0, // Drawing is faster without shadows
		},
		yaxis: {
			show: true,
			tickFormatter: function(val, axis) { return APP.formatNumber(val)+'V'; }
		},
		xaxis: {
			show: true,
			tickSize: APP.signals.length_display/10,
			tickFormatter: function(val, axis) {
				//return APP.formatNumber(val)+'s';
				return APP.formatNumber((val/APP.signals.length_display) * APP.getVal('PLOT_X_MAX'))+'s';
			}
		},
		grid: {
			show: true
		}
	});

	// Bind to the window resize event to redraw the graph; trigger that event to do the first drawing
	$(window).resize(function() {
		if (APP.ws) {
			APP.sendParameters();
		}
		APP.plot.resize();
		APP.plot.setupGrid();
		APP.plot.draw();
	}).resize();


	// Stop the application when page is unloaded
	$(window).on('beforeunload', function() {
		APP.ws.onclose = function() {}; // disable onclose handler first
		APP.ws.close();
		$.ajax({
			url: APP.config.stop_app_url,
			async: false
		});
		APP.vars.unexpectedClose = false;
	});

	for (var name in APP.UI.changeCallbacks) {
		$("#" + name).change(APP.UI.changeCallbacks[name]);
	}
	for (var name in APP.UI.clickCallbacks) {
		$("#" + name).click(APP.UI.clickCallbacks[name]);
	}

	// Revenir à la page précédente si on clic sur back-btn
	APP.config.previousPageUrl = document.referrer;
	const currentUrl = window.location.href;
	if (currentUrl === APP.config.previousPageUrl || APP.config.previousPageUrl === ""){
		APP.config.previousPageUrl = "/";
	}

	$("#return-button").on("click", function(event) {
		APP.vars.unexpected_closed = false;
		window.location.href = APP.config.previousPageUrl;
	});
	$("#home-button").on("click", function(event) {
		APP.vars.unexpected_closed = false;
		window.location.href = "/";
		APP.ws.onclose = function() {}; // disable onclose handler first
		APP.ws.close();
	});

	APP.UI.close_menu();
	APP.UI.update_plot();

	// Init help
	Help.init(helpListSM);
	Help.setState("idle");

	APP.startApp();

});
