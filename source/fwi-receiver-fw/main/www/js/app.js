let currentData =
{
	is_connected: false,

	status: {
		req: 0,
		slots: []
	},
};

var currentApp = new Vue({
  el: '#app',
  data: currentData,
  methods: {
	idBtnReboot_Click(event)
	{
		SendAction("/action/reboot");
	},
	idBtnCheckIgnition_Click(event)
	{
		SendAction("/action/checkignition");
	},
	idBtnDoorTest_Click(event)
	{
		SendAction("/action/doortest");
	},
	idBtnDisarm_Click(event)
	{
		SendAction("/action/disarm");
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
	await fetch('/api/getstatus')
		.then((response) => {
			if (!response.ok) {
				throw new Error('Unable to get status');
			}
			return response.json();
		})
		.then((data) =>
		{
		  //console.log("data: ", data);
		  currentData.status = data.status;
		  currentData.is_connected = true;
		  setTimeout(timerHandler, 500);
		})
		.catch((ex) =>
		{
			setTimeout(timerHandler, 5000);
			currentData.is_connected = false;
			console.error('getstatus', ex);
		});
  }
