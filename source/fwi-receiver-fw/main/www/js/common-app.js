const EOutputState = {
	Idle: 0,
	Enabled: 1,
	Fired: 2,
	Connected: 3
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
