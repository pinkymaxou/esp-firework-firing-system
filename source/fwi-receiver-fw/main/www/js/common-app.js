
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
