const EOutputState = {
	Idle: 0,
	Enabled: 1,
	Fired: 2,
	Connected: 3
};

const EGeneralState = {
	Idle: 0,
	FiringMasterSwitchWrongStateError: 1,
	FiringUnknownError: 2,
	Firing: 3,
	FiringOK: 4,
	ArmingSystemOK: 7,
	CheckingConnection: 8,
	CheckingConnectionOK: 9,
	CheckingConnectionError: 10,
	LiveCheckContinuity: 11,
	DisarmedMasterSwitchOff: 12
};

function SendAction(actionURL, data)
{
	var xhr = new XMLHttpRequest();
	xhr.open("POST", actionURL, true);
	if (data) {
		xhr.setRequestHeader('Content-Type', 'application/json');
		xhr.send(JSON.stringify(data));
	}
	else {
		xhr.send(null);
	}
}
