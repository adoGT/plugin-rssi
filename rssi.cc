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

#include <packetsource.h>
#include <packetchain.h>
#include <dumpfile.h>

//minimal includes
#include <plugintracker.h>
#include <globalregistry.h>
#include <version.h>

#define VERSION_RSSI_MAJOR	"0"
#define VERSION_RSSI_MINOR	"0"
#define VERSION_RSSI_TINY	"0"

GlobalRegistry *globalreg = NULL;

class Dumpfile_Rssitxt : public Dumpfile {
public:
	Dumpfile_Rssitxt() {
		fprintf(stderr, "FATAL OOPS: dumpfile_rssitxt()\n"); exit(1); 
	}

	Dumpfile_Rssitxt(GlobalRegistry *in_globalreg);
	virtual ~Dumpfile_Rssitxt();
        
        virtual int chain_handler(kis_packet *in_pack);
	virtual int Flush();

protected:
	FILE *txtfile;
	time_t last_track;
	int pack_comp_common;
};

int dumpfilerssitxt_chain_hook(CHAINCALL_PARMS) {
	return ((Dumpfile_Rssitxt *) auxdata)->chain_handler(in_pack);
}

Dumpfile_Rssitxt::Dumpfile_Rssitxt(GlobalRegistry *in_globalreg) : 
	Dumpfile(in_globalreg) {
    
	globalreg = in_globalreg;

	pack_comp_common = 
		globalreg->packetchain->RegisterPacketComponent("COMMON");

	txtfile = NULL;

	last_track = 0;

	type = "rssitxt";
        
	// Find the file name
	if ((fname = ProcessConfigOpt("rssitxt")) == "" ||
		globalreg->fatal_condition) {
                _MSG("RSSI plugin is OFF.",
			 MSGFLAG_INFO);
		return;
	}
        
	if ((txtfile = fopen(fname.c_str(), "w")) == NULL) {
		_MSG("Failed to open rssitxt log file '" + fname + "': " + strerror(errno),
			 MSGFLAG_FATAL);
		globalreg->fatal_condition = 1;
		return;
	}

	fprintf(txtfile, "#device,dest,ts,ts_usec,lat,lon,spd,heading,alt,hdop,vdop,"
			"fix,sigtype,signal,noise,phy,packtype\n");

	_MSG("Opened rssitxt log file '" + fname + "'", MSGFLAG_INFO);

	globalreg->packetchain->RegisterHandler(&dumpfilerssitxt_chain_hook, this,
            CHAINPOS_LOGGING, -100);

	globalreg->RegisterDumpFile(this);
}

Dumpfile_Rssitxt::~Dumpfile_Rssitxt() {
	globalreg->packetchain->RemoveHandler(&dumpfilerssitxt_chain_hook,
            CHAINPOS_LOGGING);

	// Close files
	if (txtfile != NULL) {
		Flush();
		fclose(txtfile);
	}

	txtfile = NULL;
}

int Dumpfile_Rssitxt::Flush() {
	if (txtfile == NULL)
		return 0;

	fflush(txtfile);

	return 1;
}

int Dumpfile_Rssitxt::chain_handler(kis_packet *in_pack) {
	kis_layer1_packinfo *radio = NULL;
	kis_ieee80211_packinfo *eight11 = NULL;
	kis_ref_capsource *csrc_ref = NULL;

	if(in_pack->error)
		return 0;

	eight11 = (kis_ieee80211_packinfo *) in_pack->fetch(_PCM(PACK_COMP_80211));
	if(eight11 == NULL)
		return 0;

	csrc_ref = (kis_ref_capsource *) in_pack->fetch(_PCM(PACK_COMP_KISCAPSRC));
	if(csrc_ref == NULL && csrc_ref->ref_source == NULL)
		return 0;

	if(eight11->corrupt || eight11->type == packet_unknown)
		return 0;

	radio = (kis_layer1_packinfo *) in_pack->fetch(_PCM(PACK_COMP_RADIODATA));

	int rtype = 0, sig = 0, noise = 0;

	if (radio != NULL) {
		if (radio->signal_rssi != 0) {
			rtype = 1;
			sig = radio->signal_rssi;
			noise = radio->noise_rssi;
		} else {
			rtype = 2;
			sig = radio->signal_dbm;
			noise  = radio->noise_dbm;
		}
	}

	fprintf(txtfile, "%s (%s) %s,%s,%s,%s,%ld,%ld,%d,%d,%d\n",
		csrc_ref->ref_source->FetchName().c_str(),
		csrc_ref->ref_source->FetchInterface().c_str(),
		csrc_ref->ref_source->FetchUUID().UUID2String().c_str(),
		eight11->bssid_mac.Mac2String().c_str(),
		eight11->source_mac.Mac2String().c_str(),
		eight11->dest_mac.Mac2String().c_str(),
		(long int) in_pack->ts.tv_sec, (long int) in_pack->ts.tv_usec,
		rtype, sig, noise);

	dumped_frames++;
	
	return 1;
}

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
        
        new Dumpfile_Rssitxt(globalreg);

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
			prev->major = string(VERSION_MAJOR);
			prev->minor = string(VERSION_MINOR);
			prev->tiny = string(VERSION_TINY);
		}
	}
}

