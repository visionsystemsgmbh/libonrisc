#include "napi.h"
#include <stdio.h>
#include <string.h>
#include <onrisc.h>

class OnriscSystem : public Napi::ObjectWrap<OnriscSystem> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "OnriscSystem", {
            InstanceMethod("getHwParams", &OnriscSystem::GetHwParams),
            InstanceMethod("getWlanSwState", &OnriscSystem::GetWlanSwState),
            InstanceMethod("setMpcieSwState", &OnriscSystem::SetMpcieSwState),
            InstanceMethod("setUartMode", &OnriscSystem::SetUartMode),
            InstanceMethod("getUartMode", &OnriscSystem::GetUartMode),
            InstanceMethod("setGpioValue", &OnriscSystem::SetGpioValue),
            InstanceMethod("getGpioValue", &OnriscSystem::GetGpioValue),
            InstanceMethod("switchLed", &OnriscSystem::SwitchLed),
            InstanceMethod("getDips", &OnriscSystem::GetDips),
        });
        exports.Set("OnriscSystem", func);

	// LED enum
	Napi::Object enumLedObj = Napi::Object::New(env);
	enumLedObj.Set("LED_POWER", Napi::Number::New(env, LED_POWER));
	enumLedObj.Set("LED_WLAN", Napi::Number::New(env, LED_WLAN));
	enumLedObj.Set("LED_APP", Napi::Number::New(env, LED_APP));
        exports.Set("LedType", enumLedObj);

	// RS mode enum
	Napi::Object enumRsModesObj = Napi::Object::New(env);
	enumRsModesObj.Set("TYPE_UNKNOWN", Napi::Number::New(env, TYPE_UNKNOWN));
	enumRsModesObj.Set("TYPE_RS232", Napi::Number::New(env, TYPE_RS232));
	enumRsModesObj.Set("TYPE_RS422", Napi::Number::New(env, TYPE_RS422));
	enumRsModesObj.Set("TYPE_RS485_FD", Napi::Number::New(env, TYPE_RS485_FD));
	enumRsModesObj.Set("TYPE_RS485_HD", Napi::Number::New(env, TYPE_RS485_HD));
	enumRsModesObj.Set("TYPE_LOOPBACK", Napi::Number::New(env, TYPE_LOOPBACK));
	enumRsModesObj.Set("TYPE_DIP", Napi::Number::New(env, TYPE_DIP));
	enumRsModesObj.Set("TYPE_CAN", Napi::Number::New(env, TYPE_CAN));
        exports.Set("RsModes", enumRsModesObj);

        return exports;
    }

    OnriscSystem(const Napi::CallbackInfo& info) : Napi::ObjectWrap<OnriscSystem>(info) {
        // Initialize the C struct here
        memset(&system_data, 0, sizeof(onrisc_system_t));
	onrisc_init(&system_data);
	onrisc_blink_create(&pwr);
	pwr.led_type = LED_POWER;
	onrisc_blink_create(&app);
	app.led_type = LED_APP;
	onrisc_blink_create(&wln);
	wln.led_type = LED_WLAN;
    }

private:
    Napi::Value GetHwParams(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	char tmp[64];

        // Create an object literal
        Napi::Object result = Napi::Object::New(env);
        result.Set("model", Napi::Number::New(env, system_data.model));
        result.Set("ser_nr", Napi::Number::New(env, system_data.ser_nr));
        result.Set("prd_date", Napi::String::New(env, system_data.prd_date));
	sprintf(tmp, "%d.%d", system_data.hw_rev >> 16,
		system_data.hw_rev & 0xff);
        result.Set("hw_rev", Napi::String::New(env, tmp));
	sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
		system_data.mac1[0],
		system_data.mac1[1],
		system_data.mac1[2],
		system_data.mac1[3],
		system_data.mac1[4],
		system_data.mac1[5]);
        result.Set("mac1", Napi::String::New(env, tmp));
	sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
		system_data.mac2[0],
		system_data.mac2[1],
		system_data.mac2[2],
		system_data.mac2[3],
		system_data.mac2[4],
		system_data.mac2[5]);
        result.Set("mac2", Napi::String::New(env, tmp));
	sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
		system_data.mac3[0],
		system_data.mac3[1],
		system_data.mac3[2],
		system_data.mac3[3],
		system_data.mac3[4],
		system_data.mac3[5]);
        result.Set("mac3", Napi::String::New(env, tmp));

        return result;
    }

    Napi::Value GetWlanSwState(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	gpio_level wlan_sw_state;
	if (onrisc_get_wlan_sw_state(&wlan_sw_state) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to get WLAN switch state")
        	.ThrowAsJavaScriptException();
    		return env.Null();
	}

	return Napi::Number::New(env, wlan_sw_state);
    }

    void SetMpcieSwState(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	gpio_level sw_state = HIGH;
	if (info.Length() != 1) {
    		Napi::TypeError::New(env, "Wrong number of arguments")
        	.ThrowAsJavaScriptException();
    		return;
  	}
	int arg0 = info[0].As<Napi::Number>().Int32Value();
	if (!arg0)
		sw_state = LOW;

	if (onrisc_set_mpcie_sw_state(sw_state) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to set the mPCIe switch")
        	.ThrowAsJavaScriptException();
    		return;
	}
    }

    Napi::Value GetUartMode(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        Napi::Object result = Napi::Object::New(env);
	gpio_level term_state = HIGH;
	onrisc_uart_mode_t mode;

	if (info.Length() != 1) {
    		Napi::TypeError::New(env, "Wrong number of arguments")
        	.ThrowAsJavaScriptException();
    		return env.Null();
  	}
	int port_nr = info[0].As<Napi::Number>().Int32Value();

	if (onrisc_get_uart_mode(port_nr, &mode) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to get UART mode")
        	.ThrowAsJavaScriptException();
    		return env.Null();
	}

	if (mode.termination == LOW)
		term_state = LOW;

        result.Set("rs_mode", Napi::Number::New(env, mode.rs_mode));
        result.Set("termination", Napi::Number::New(env, term_state));

        return result;
    }

    void SetUartMode(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	gpio_level term_state = HIGH;
	onrisc_uart_mode_t mode;
	if (info.Length() != 3) {
    		Napi::TypeError::New(env, "Wrong number of arguments")
        	.ThrowAsJavaScriptException();
    		return;
  	}
	int port_nr = info[0].As<Napi::Number>().Int32Value();
	int rs_mode = info[1].As<Napi::Number>().Int32Value();
	int termination = info[2].As<Napi::Number>().Int32Value();
	if (!termination)
		term_state = LOW;

	mode.rs_mode = rs_mode;
	mode.termination = term_state;

	if (onrisc_set_uart_mode(port_nr, &mode) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to set UART mode")
        	.ThrowAsJavaScriptException();
    		return;
	}
    }

    Napi::Value GetGpioValue(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        Napi::Object result = Napi::Object::New(env);
	onrisc_gpios_t gpio_val;

	if (onrisc_gpio_get_value(&gpio_val) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to get GPIO value")
        	.ThrowAsJavaScriptException();
    		return env.Null();
	}

        result.Set("mask", Napi::Number::New(env, gpio_val.mask));
        result.Set("value", Napi::Number::New(env, gpio_val.value));

        return result;
    }

    void SetGpioValue(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	onrisc_gpios_t gpio_val;

	if (info.Length() != 2) {
    		Napi::TypeError::New(env, "Wrong number of arguments")
        	.ThrowAsJavaScriptException();
    		return;
  	}

	gpio_val.mask = info[0].As<Napi::Number>().Int32Value();
	gpio_val.value = info[1].As<Napi::Number>().Int32Value();

	if (onrisc_gpio_set_value(&gpio_val) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to set GPIO value")
        	.ThrowAsJavaScriptException();
    		return;
	}
    }

    void SwitchLed(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	blink_led_t *led;

	if (info.Length() != 2) {
    		Napi::TypeError::New(env, "Wrong number of arguments")
        	.ThrowAsJavaScriptException();
    		return;
  	}

	int led_type = info[0].As<Napi::Number>().Int32Value();
	int led_state = info[1].As<Napi::Number>().Int32Value();

	switch(led_type) {
		case LED_POWER:
			led = &pwr;
			break;
		case LED_APP:
			led = &app;
			break;
		case LED_WLAN:
			led = &wln;
			break;
		default:
			Napi::TypeError::New(env, "Unknown LED type")
			.ThrowAsJavaScriptException();
			return;
	}

	if (onrisc_switch_led(led, led_state) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to switch LED")
        	.ThrowAsJavaScriptException();
    		return;
	}
    }

    Napi::Value GetDips(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
	uint32_t dips;

	if (onrisc_get_dips(&dips) == EXIT_FAILURE) {
    		Napi::TypeError::New(env, "Failed to get DIPs")
        	.ThrowAsJavaScriptException();
    		return env.Null();
	}

	return Napi::Number::New(env, dips);
    }

private:
    onrisc_system_t system_data;
    blink_led_t pwr;
    blink_led_t app;
    blink_led_t wln;

};

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
	  return OnriscSystem::Init(env, exports);
}

NODE_API_MODULE(libonrisc, InitAll)
