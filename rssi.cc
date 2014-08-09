/*
    Copyright 2014 Adrian Farmadin
 
    This file is part of plugin-rssi.

    Plugin-rssi is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Plugin-rssi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with plugin-rssi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <messagebus.h>

#include <plugintracker.h>
#include <globalregistry.h>

#define VERSION_RSSI_MAJOR	"0"
#define VERSION_RSSI_MINOR	"0"
#define VERSION_RSSI_TINY	"0"

GlobalRegistry *globalreg = NULL;

int rssi_unregister(GlobalRegistry *in_globalreg) {
	return 0;
}

int rssi_register(GlobalRegistry *in_globalreg) {
	globalreg = in_globalreg;

	if (globalreg->kismet_instance != KISMET_INSTANCE_SERVER) {
		_MSG("Not initializing RSSI, not running on a server.",
			 MSGFLAG_INFO);
		return 1;
	}

	return 1;
}

extern "C" {
	int kis_plugin_info(plugin_usrdata *data) {
		data->pl_name = "RSSI";
		data->pl_version = string(VERSION_RSSI_MAJOR) + "." + string(VERSION_RSSI_MINOR) + "." + string(VERSION_RSSI_TINY);
		data->pl_description = "RSSI Plugin";
		data->pl_unloadable = 0;
		data->plugin_register = rssi_register;
		data->plugin_unregister = rssi_unregister;

		return 1;
	}

	void kis_revision_info(plugin_revision *prev) {
		if (prev->version_api_revision >= 1) {
			prev->version_api_revision = 1;
			prev->major = string(VERSION_RSSI_MAJOR);
			prev->minor = string(VERSION_RSSI_MINOR);
			prev->tiny = string(VERSION_RSSI_TINY);
		}
	}
}

