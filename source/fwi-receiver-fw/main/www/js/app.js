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
	idBtnCheckIgnition_Click(event)
	{
		SendAction("/action/checkconnections");
	},
	idBtnDisarm_Click(event)
	{
		SendAction("/action/disarmfiringsystem");
	},
	idBtnArm_Click(event)
	{
		SendAction("/action/armfiringsystem");
	},
	idBtnFire_Click(ix)
	{
		SendAction("/action/firesystem");
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
