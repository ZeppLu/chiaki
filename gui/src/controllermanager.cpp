// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <controllermanager.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QByteArray>
#include <QTimer>

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
#include <SDL.h>
#endif

static QSet<QString> chiaki_motion_controller_guids({
	// Sony on Linux
	"03000000341a00003608000011010000",
	"030000004c0500006802000010010000",
	"030000004c0500006802000010810000",
	"030000004c0500006802000011010000",
	"030000004c0500006802000011810000",
	"030000006f0e00001402000011010000",
	"030000008f0e00000300000010010000",
	"050000004c0500006802000000010000",
	"050000004c0500006802000000800000",
	"050000004c0500006802000000810000",
	"05000000504c415953544154494f4e00",
	"060000004c0500006802000000010000",
	"030000004c050000a00b000011010000",
	"030000004c050000a00b000011810000",
	"030000004c050000c405000011010000",
	"030000004c050000c405000011810000",
	"030000004c050000cc09000000010000",
	"030000004c050000cc09000011010000",
	"030000004c050000cc09000011810000",
	"03000000c01100000140000011010000",
	"050000004c050000c405000000010000",
	"050000004c050000c405000000810000",
	"050000004c050000c405000001800000",
	"050000004c050000cc09000000010000",
	"050000004c050000cc09000000810000",
	"050000004c050000cc09000001800000",
	// Sony on iOS
	"050000004c050000cc090000df070000",
	// Sony on Android
	"050000004c05000068020000dfff3f00",
	"030000004c050000cc09000000006800",
	"050000004c050000c4050000fffe3f00",
	"050000004c050000cc090000fffe3f00",
	"050000004c050000cc090000ffff3f00",
	"35643031303033326130316330353564",
	// Sony on Mac OSx
	"030000004c050000cc09000000000000",
	"030000004c0500006802000000000000",
	"030000004c0500006802000000010000",
	"030000004c050000a00b000000010000",
	"030000004c050000c405000000000000",
	"030000004c050000c405000000010000",
	"030000004c050000cc09000000010000",
	"03000000c01100000140000000010000",
	// Sony on Windows
	"030000004c050000a00b000000000000",
	"030000004c050000c405000000000000",
	"030000004c050000cc09000000000000",
	"03000000250900000500000000000000",
	"030000004c0500006802000000000000",
	"03000000632500007505000000000000",
	"03000000888800000803000000000000",
	"030000008f0e00001431000000000000",
});

static ControllerManager *instance = nullptr;

#define UPDATE_INTERVAL_MS 4

ControllerManager *ControllerManager::GetInstance()
{
	if(!instance)
		instance = new ControllerManager(qApp);
	return instance;
}

ControllerManager::ControllerManager(QObject *parent)
	: QObject(parent)
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
	{
		const char *err = SDL_GetError();
		QMessageBox::critical(nullptr, "SDL Init", tr("Failed to initialized SDL Gamecontroller: %1").arg(err ? err : ""));
	}

	auto timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &ControllerManager::HandleEvents);
	timer->start(UPDATE_INTERVAL_MS);
#endif

	UpdateAvailableControllers();
}

ControllerManager::~ControllerManager()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_Quit();
#endif
}

void ControllerManager::UpdateAvailableControllers()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	QSet<SDL_JoystickID> current_controllers;
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{
		if(!SDL_IsGameController(i))
			continue;

		// We'll try to identify pads with Motion Control
		SDL_JoystickGUID guid = SDL_JoystickGetGUID(SDL_JoystickOpen(i));
		char guid_str[256];
		SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));

		if(chiaki_motion_controller_guids.contains(guid_str))
		{
			SDL_Joystick *joy = SDL_JoystickOpen(i);
			if(joy)
			{
				bool no_buttons = SDL_JoystickNumButtons(joy) == 0;
				SDL_JoystickClose(joy);
				if(no_buttons)
					continue;
			}
		}

		current_controllers.insert(SDL_JoystickGetDeviceInstanceID(i));
	}

	if(current_controllers != available_controllers)
	{
		available_controllers = current_controllers;
		emit AvailableControllersUpdated();
	}
#endif
}

void ControllerManager::HandleEvents()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				UpdateAvailableControllers();
				break;
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERAXISMOTION:
#if !defined(CHIAKI_ENABLE_SETSU) && SDL_VERSION_ATLEAST(2, 0, 14)
			case SDL_CONTROLLERSENSORUPDATE:
			case SDL_CONTROLLERTOUCHPADDOWN:
			case SDL_CONTROLLERTOUCHPADMOTION:
			case SDL_CONTROLLERTOUCHPADUP:
#endif
				ControllerEvent(event);
				break;
		}
	}
#endif
}

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
void ControllerManager::ControllerEvent(SDL_Event event)
{
	int device_id;
	switch(event.type)
	{
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			device_id = event.cbutton.which;
			break;
		case SDL_CONTROLLERAXISMOTION:
			device_id = event.caxis.which;
			break;
#if SDL_VERSION_ATLEAST(2, 0, 14)
		case SDL_CONTROLLERSENSORUPDATE:
			device_id = event.csensor.which;
			break;
		case SDL_CONTROLLERTOUCHPADDOWN:
		case SDL_CONTROLLERTOUCHPADMOTION:
		case SDL_CONTROLLERTOUCHPADUP:
			device_id = event.ctouchpad.which;
			break;
#endif
		default:
			return;
	}
	if(!open_controllers.contains(device_id))
		return;
	open_controllers[device_id]->UpdateState(event);
}
#endif

QSet<int> ControllerManager::GetAvailableControllers()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return available_controllers;
#else
	return {};
#endif
}

Controller *ControllerManager::OpenController(int device_id, ChiakiLog *log)
{
	if(open_controllers.contains(device_id))
		return nullptr;
	auto controller = new Controller(device_id, this, log);
	open_controllers[device_id] = controller;
	return controller;
}

void ControllerManager::ControllerClosed(Controller *controller)
{
	open_controllers.remove(controller->GetDeviceID());
}

Controller::Controller(int device_id, ControllerManager *manager, ChiakiLog *log) : QObject(manager)
{
	printf("zepp id: %d\n", id);
	this->id = device_id;
	this->manager = manager;
	this->log = log;
	chiaki_orientation_tracker_init(&this->orientation_tracker);
	chiaki_controller_state_set_idle(&this->state);

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	controller = nullptr;
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{
		CHIAKI_LOGI(this->log, "zepp sdl index: %d", i);
		CHIAKI_LOGI(this->log, "zepp sdl name: %s", SDL_GameControllerNameForIndex(i));
		if(SDL_JoystickGetDeviceInstanceID(i) == device_id)
		{
			CHIAKI_LOGI(this->log, "zepp sdl matched");
			controller = SDL_GameControllerOpen(i);
#if SDL_VERSION_ATLEAST(2, 0, 14)
			bool has_accel = SDL_GameControllerHasSensor(controller, SDL_SENSOR_ACCEL);
			int en_accel = 0;
			if(has_accel)
				en_accel = SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_ACCEL, SDL_TRUE);
			if(en_accel < 0) {
				CHIAKI_LOGE(this->log, "zepp err setting accel: %s", SDL_GetError());
			}
			bool has_gyro = SDL_GameControllerHasSensor(controller, SDL_SENSOR_GYRO);
			int en_gyro = 0;
			if(has_gyro)
				en_gyro = SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_GYRO, SDL_TRUE);
			if(en_gyro < 0) {
				CHIAKI_LOGE(this->log, "zepp err setting gyro: %s", SDL_GetError());
			}
			CHIAKI_LOGI(this->log, "zepp controller accel=%d, gyro=%d", has_accel, has_gyro);
			CHIAKI_LOGI(this->log, "zepp controller enable accel=%d, gyro=%d", en_accel, en_gyro);
			break;
#endif
		}
	}
#endif
}

Controller::~Controller()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(controller)
		SDL_GameControllerClose(controller);
#endif
	manager->ControllerClosed(this);
}

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
void Controller::UpdateState(SDL_Event event)
{
	switch(event.type)
	{
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			if(!HandleButtonEvent(event.cbutton))
				return;
			break;
		case SDL_CONTROLLERAXISMOTION:
			if(!HandleAxisEvent(event.caxis))
				return;
			break;
#if SDL_VERSION_ATLEAST(2, 0, 14)
		case SDL_CONTROLLERSENSORUPDATE:
			if(!HandleSensorEvent(event.csensor))
				return;
			break;
		case SDL_CONTROLLERTOUCHPADDOWN:
		case SDL_CONTROLLERTOUCHPADMOTION:
		case SDL_CONTROLLERTOUCHPADUP:
			if(!HandleTouchpadEvent(event.ctouchpad))
				return;
			break;
#endif
		default:
			return;

	}
	emit StateChanged();
}

inline bool Controller::HandleButtonEvent(SDL_ControllerButtonEvent event) {
	CHIAKI_LOGI(this->log, "zepp button %s=%x",
			SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(event.button)), event.type);
	ChiakiControllerButton ps_btn;
	switch(event.button)
	{
		case SDL_CONTROLLER_BUTTON_A:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_CROSS;
			break;
		case SDL_CONTROLLER_BUTTON_B:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_MOON;
			break;
		case SDL_CONTROLLER_BUTTON_X:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_BOX;
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_PYRAMID;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_DPAD_UP;
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_L1;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_R1;
			break;
		case SDL_CONTROLLER_BUTTON_LEFTSTICK:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_L3;
			break;
		case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_R3;
			break;
		case SDL_CONTROLLER_BUTTON_START:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_OPTIONS;
			break;
		case SDL_CONTROLLER_BUTTON_BACK:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_SHARE;
			break;
		case SDL_CONTROLLER_BUTTON_GUIDE:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_PS;
			break;
#if SDL_VERSION_ATLEAST(2, 0, 14)
		case SDL_CONTROLLER_BUTTON_TOUCHPAD:
			ps_btn = CHIAKI_CONTROLLER_BUTTON_TOUCHPAD;
			break;
#endif
		default:
			return false;
		}
	if(event.type == SDL_CONTROLLERBUTTONDOWN)
		state.buttons |= ps_btn;
	else
		state.buttons &= ~ps_btn;
	return true;
}

inline bool Controller::HandleAxisEvent(SDL_ControllerAxisEvent event) {
	CHIAKI_LOGI(this->log, "zepp axis %s=%d",
			SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(event.axis)), event.value);
	switch(event.axis)
	{
		case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
			state.l2_state = (uint8_t)(event.value >> 7);
			break;
		case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
			state.r2_state = (uint8_t)(event.value >> 7);
			break;
		case SDL_CONTROLLER_AXIS_LEFTX:
			state.left_x = event.value;
			break;
		case SDL_CONTROLLER_AXIS_LEFTY:
			state.left_y = event.value;
			break;
		case SDL_CONTROLLER_AXIS_RIGHTX:
			state.right_x = event.value;
			break;
		case SDL_CONTROLLER_AXIS_RIGHTY:
			state.right_y = event.value;
			break;
		default:
			return false;
	}
	return true;
}

#if SDL_VERSION_ATLEAST(2, 0, 14)
inline bool Controller::HandleSensorEvent(SDL_ControllerSensorEvent event)
{
	CHIAKI_LOGI(this->log, "zepu sensor (accel=%d gyro=%d): (%f, %f, %f)",
			event.sensor == SDL_SENSOR_ACCEL ? 1 : 0,
			event.sensor == SDL_SENSOR_GYRO ? 1 : 0,
			event.data[0], event.data[1], event.data[2]);
	switch(event.sensor)
	{
		case SDL_SENSOR_ACCEL:
			state.accel_x = event.data[0] / SDL_STANDARD_GRAVITY;
			state.accel_y = event.data[1] / SDL_STANDARD_GRAVITY;
			state.accel_z = event.data[2] / SDL_STANDARD_GRAVITY;
			break;
		case SDL_SENSOR_GYRO:
			state.gyro_x = event.data[0];
			state.gyro_y = event.data[1];
			state.gyro_z = event.data[2];
			break;
		default:
			return false;
	}
	chiaki_orientation_tracker_update(
		&orientation_tracker, state.gyro_x, state.gyro_y, state.gyro_z,
		state.accel_x, state.accel_y, state.accel_z, event.timestamp * 1000);
	chiaki_orientation_tracker_apply_to_controller_state(&orientation_tracker, &state);
	return true;
}

inline bool Controller::HandleTouchpadEvent(SDL_ControllerTouchpadEvent event)
{
	CHIAKI_LOGI(this->log, "zepu touchpad: (%f, %f)", event.x, event.y);
	auto key = qMakePair(event.touchpad, event.finger);
	bool exists = touch_ids.contains(key);
	uint8_t chiaki_id;
	switch(event.type)
	{
		case SDL_CONTROLLERTOUCHPADDOWN:
			if(touch_ids.size() >= CHIAKI_CONTROLLER_TOUCHES_MAX)
				return false;
			chiaki_id = chiaki_controller_state_start_touch(&state, event.x * PS_TOUCHPAD_MAX_X, event.y * PS_TOUCHPAD_MAX_Y);
			touch_ids.insert(key, chiaki_id);
			break;
		case SDL_CONTROLLERTOUCHPADMOTION:
			if(!exists)
				return false;
			chiaki_controller_state_set_touch_pos(&state, touch_ids[key], event.x * PS_TOUCHPAD_MAX_X, event.y * PS_TOUCHPAD_MAX_Y);
			break;
		case SDL_CONTROLLERTOUCHPADUP:
			if(!exists)
				return false;
			chiaki_controller_state_stop_touch(&state, touch_ids[key]);
			touch_ids.remove(key);
			break;
		default:
			return false;
	}
	return true;
}
#endif
#endif

bool Controller::IsConnected()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return controller && SDL_GameControllerGetAttached(controller);
#else
	return false;
#endif
}

int Controller::GetDeviceID()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return id;
#else
	return -1;
#endif
}

QString Controller::GetName()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(!controller)
		return QString();
	SDL_Joystick *js = SDL_GameControllerGetJoystick(controller);
	SDL_JoystickGUID guid = SDL_JoystickGetGUID(js);
	char guid_str[256];
	SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));
	return QString("%1 (%2)").arg(SDL_JoystickName(js), guid_str);
#else
	return QString();
#endif
}

ChiakiControllerState Controller::GetState()
{
	return state;
}

void Controller::SetRumble(uint8_t left, uint8_t right)
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(!controller)
		return;
	SDL_GameControllerRumble(controller, (uint16_t)left << 8, (uint16_t)right << 8, 5000);
#endif
}
