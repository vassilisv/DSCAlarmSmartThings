//
// In device configuration select the following capabilities: switch, contact and alarm
//

metadata {
	// Simulator metadata
	simulator {

	}

	// UI tile definitions
	tiles {
		standardTile("switch", "device.switch", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
			state "on", label: '${name}', action: "switch.off", icon: "st.switches.switch.on", backgroundColor: "#79b821"
			state "off", label: '${name}', action: "switch.on", icon: "st.switches.switch.off", backgroundColor: "#ffffff"
		}
        standardTile("contact", "device.contact", width: 1, height: 1) {
			state "open", label: '${name}', icon: "st.contact.contact.open", backgroundColor: "#ffa81e"
			state "closed", label: '${name}', icon: "st.contact.contact.closed", backgroundColor: "#79b821"
		}
		standardTile("alarm", "device.alarm", width: 1, height: 1) {
			state "off", label:'off', action:'alarm.both', icon:"st.alarm.alarm.alarm", backgroundColor:"#ffffff"
			state "strobe", label:'strobe!', action:'clear', icon:"st.alarm.alarm.alarm", backgroundColor:"#e86d13"
			state "siren", label:'siren!', action:'clear', icon:"st.alarm.alarm.alarm", backgroundColor:"#e86d13"
			state "both", label:'alarm!', action:'clear', icon:"st.alarm.alarm.alarm", backgroundColor:"#e86d13"
		}
		main(["switch","contact","alarm"])
		details(["switch","contact","alarm"])
	}
}

// Parse incoming device messages to generate events
def parse(String description) {
	def msg = zigbee.parse(description)?.text
    def result
    if ( msg.length() >= 4 ) {
        // Process arm update
        if ( msg.substring(0, 2) == "AR" ) {
            result = createEvent(name: "switch", value: msg[3] == "1" ? "on":"off")
        // Process alarm update
        } else if ( msg.substring(0, 2) == "AL" ) {
            result = createEvent(name: "alarm", value: msg[3] == "1" ? "both":"off")
        // Process zone update
        } else if ( msg.substring(0, 2) == "ZN" ) {
            def state = "closed"
            for (int n = 0; n < 8; ++n) {
                state = (msg[3+n] == "1" ? "open":state)
            }
            result = createEvent(name: "contact", value: state)
        }
    }
	log.debug "Parse returned ${result?.descriptionText}"
	return result
}

// Commands sent to the device
def on() {
	zigbee.smartShield(text: "arm").format()
}

def off() {
	zigbee.smartShield(text: "disarm").format()
}

def strobe() {
	panic()
}

def siren() {
	panic()
} 

def both() {
	panic()
}

def panic() {
	zigbee.smartShield(text: "panic").format()
}

// TODO: Need to send off, on, off with a few secs in between to stop and clear the alarm
def clear() {
	off()
}
