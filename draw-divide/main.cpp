#include <Geode/Geode.hpp>

using namespace geode::prelude;

// ↓↓↓ ESTO ES LO NUEVO ↓↓↓
#ifdef GEODE_IS_WINDOWS
#include <windows.h>

double get_refresh_rate() {
	static const double refresh_rate = [] {
		DEVMODEA device_mode;
		memset(&device_mode, 0, sizeof(device_mode));
		device_mode.dmSize = sizeof(device_mode);
		device_mode.dmDriverExtra = 0;

		if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &device_mode) == 0) {
			return 60.0;
		} else {
			auto freq = device_mode.dmDisplayFrequency;
			if (device_mode.dmDisplayFlags & DM_INTERLACED) {
				freq *= 2;
			}
			return static_cast<double>(freq);
		}
	}();
	return refresh_rate;
}
#else
// Para Android, Mac o Linux simplemente devolvemos 60
double get_refresh_rate() {
    return 60.0;
}
#endif
// ↑↑↑ FIN DE LO NUEVO ↑↑↑

double g_delta_count = 0.0;
bool g_enabled = true;
bool g_force_target_fps = false;
int g_force_fps = 60;

#ifdef DEBUG_FPS
int count_total = 0;
int count_render = 0;
double total_time = 0.0;
#endif

#include <Geode/modify/CCDirector.hpp>
class $modify(CCDirector) {
	void drawScene() {
		if (!g_enabled || this->getTotalFrames() < 150) {
			return CCDirector::drawScene();
		}

		const double target_fps = g_force_target_fps ? static_cast<double>(g_force_fps) : get_refresh_rate();
		const double target_delta = 1.0 / target_fps;

		g_delta_count += this->getActualDeltaTime();

#ifdef DEBUG_FPS
		++count_total;
		total_time += this->getActualDeltaTime();
#endif

		if (g_delta_count >= target_delta) {
			g_delta_count -= target_delta;
#ifdef DEBUG_FPS
			++count_render;
#endif
			return CCDirector::drawScene();
		}

#ifdef DEBUG_FPS
		if (total_time >= 1.f) {
			total_time = 0.0;
			log::debug("{} fps, {} tps", count_render, count_total);
			count_render = 0;
			count_total = 0;
		}
#endif

		if (!this->isPaused()) {
			this->CCDirector::getScheduler()->CCScheduler::update(this->CCDirector::getDeltaTime());
		}

		if (this->getNextScene()) {
			this->setNextScene();
		}
	}
};

$on_mod(Loaded) {
	listenForSettingChanges<bool>("enabled", [](bool value) {
		g_enabled = value;
	});
	listenForSettingChanges<bool>("override-visual-fps", [](bool value) {
		g_force_target_fps = value;
	});
	listenForSettingChanges<int>("visual-fps", [](int value) {
		g_force_fps = value;
	});
}
