let currentData =
{
	is_connected: false,

	status: {
		req: 0,
		is_armed: false,
		general_state: 0,
		outputs: []
	},
};

var currentApp = new Vue({
  el: '#app',
  data: currentData,
  methods: {
	translateGeneralState(value) {
		switch(value)
		{
			case EGeneralState.Idle:
				return "Idle";
			case EGeneralState.FiringMasterSwitchWrongStateError:
				return "Firing master switch in the wrong position";
			case EGeneralState.FiringUnknownError:
				return "Unknown firing error";
			case EGeneralState.Firing:
				return "Firing";
			case EGeneralState.FiringOK:
				return "Firing OK";
			case EGeneralState.ArmingSystem:
				return "Arming the system";
			case EGeneralState.ArmingSystemNoPowerError:
				return "Cannot arm the system, no firing power";
			case EGeneralState.ArmingSystemOK:
				return "System armed and ready";
			case EGeneralState.CheckingConnection:
				return "Checking connections";
			case EGeneralState.CheckingConnectionOK:
				return "Checking connections OK";
			case EGeneralState.CheckingConnectionError:
				return "Checking connections error";
			case EGeneralState.DisarmedAutomaticTimeout:
				return "Disarmed (automatic by timeout)";
			case EGeneralState.DisarmedMasterSwitchOff:
				return "Disarmed (automatic, wrong master switch state)";
			case EGeneralState.Disarmed:
				return "Disarmed";
		}

		return 'Unknown #'+ String(value);
	},
	idBtnCheckIgnition_Click(event)
	{
		SendAction("/action/checkconnections");
	},
	idBtnFire_Click(ix)
	{
		SendAction("/action/firesystem", { index: ix });
	}
  }
})

function AppLoaded()
{
	console.log("page is fully loaded");
	setTimeout(timerHandler, 500);
}

async function timerHandler() {

	// Get system informations
	await fetch('/api/getstatus',
	{
		keepalive: true
	})
		.then((response) => {
			if (!response.ok) {
				throw new Error('Unable to get status');
			}
			return response.json();
		})
		.then((data) =>
		{
		  currentData.status = data.status;
		  currentData.is_connected = true;
		  setTimeout(timerHandler, 250);
		})
		.catch((ex) =>
		{
			setTimeout(timerHandler, 5000);
			currentData.is_connected = false;
			console.error('getstatus', ex);
		});
  }
