
var log = function(msg) {
	console.log("pebble.js: "+msg);
};
var warn = function(msg) {
	console.warn("pebble.js: "+msg);
};

// the resulting weather
var result= {};
// coordinates
var long = 0;
var lat = 0;
var curtime;
var date;
var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 

var ack = function(e) {
	log('Successfully delivered message with transactionId='+ e.data.transactionId);
};

var nack = function(e) {
	log('Unable to deliver message with transactionId='+ e.data.transactionId+ ' Error is: ' + JSON.stringify(e));
};


function locationSuccess(pos) {
	var coordinates = pos.coords;	
	lat = coordinates.latitude;	
	long = coordinates.longitude;
	log("Location success, getting weather");
	fetchWeather();
}

function locationError(err) {
  warn('location error (' + err.code + '): ' + err.message);
  if (lat !== 0) fetchWeather();  
  else {
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  }
}

// Called when JS is ready
Pebble.addEventListener("ready",
						function(e) {
							log("js is ready. Sending ok to watch");
							Pebble.sendAppMessage(
								{'0': 1},
								function(e) {
									log('Successfully delivered message with transactionId='+ e.data.transactionId);
								},
								function(e) {
									log('Unable to deliver message with transactionId='+ e.data.transactionId+ ' Error is: ' + e.error.message);
								}
							);
						});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
						function(e) {
							var payload = JSON.stringify(e.payload);
							//{"1":1,"TEMP_KEY":1}						
							log("appmessage: Received payload: " + payload);
							if (e.payload["1"] == 1) {
								navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
							} else {
								log("got msg from pebble, doing nothing");
							}
						});

function getIconCode(icon) {	
	log("icon: "+icon);
	var icode = 0;
	if (icon == '01') icode = 1;
	else if (icon == '02') icode = 2;
	else if (icon == '03') icode = 3;
	else if (icon == '04') icode = 4;
	else if (icon == '09') icode = 9;
	else if (icon == '10') icode = 10;	
	else if (icon == '11') icode = 11;	
	else if (icon == '13') icode = 13;	
	else if (icon == '50') icode = 50;	
	else icode= 50;
	log("got icon code:"+icode);
	return icode;
}
function fetchWeather() {
  var req = new XMLHttpRequest();
  result={};
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" +
    "lat=" + lat + "&lon=" + long + "&cnt=1", true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
		log("FetchWeather: \n"+req.responseText);
        var response = JSON.parse(req.responseText);
        var temperature = Math.round(response.main.temp - 273.15);               
        var weather_desc = response.weather[0].main;
		curtime = response.dt;
		date = new Date();
		result["1"]= 0;     
		result["2"]= getIconCode(response.weather[0].icon.substring(0,2));
        result["3"]= temperature+"C" + " "+weather_desc;
		fetchForecast();
      } else {
        log("Error fetching weather");		
      }
    }
  };
  req.send(null);
}

function fetchForecast() {
  var req = new XMLHttpRequest();
 
  req.open('GET', "http://api.openweathermap.org/data/2.5/forecast?" +
    "lat=" + lat + "&lon=" + long + "&cnt=3", true);
	log("fetching forecast");
  req.onload = function(e) {
    if (req.readyState == 4) {
	 
      if(req.status == 200) {
		  log("Got forecast:"+req.responseText);
        var response = JSON.parse(req.responseText);
        var list = response.list;		
        for (var i = 0; i < list.length && i < 4; i++) {
            var forecast = list[i];
			log("Forecast "+i+":"+JSON.stringify(forecast));
            var hours = Math.round((forecast.dt-curtime)/3600);
			var hoursforecast = (hours + date.getHours())%24;
			var raininfo = forecast.rain;
            var temperature = Math.round(forecast.main.temp - 273.15);
            var rain = "0";
			if (typeof(raininfo) != 'undefined') rain =  Math.round(raininfo["3h"]);           
			if (isNaN(rain)) rain = "0mm";
			else rain = rain + "mm";
            var icon = forecast.weather[0].icon.substring(0,2);
			var icode = getIconCode(icon);	
            var weather = temperature+"C "+rain+" "+hoursforecast+"h";
            var which = (i+1)*3+1;
            log("weather "+i+", which="+which+",  "+weather+", icon="+icode);             
            result[""+which] = (i+1);
            result[""+(which+1)] = icode;
            result[""+(which+2)] = weather;		
        }   
        log("sending weather to pebble:\n"+JSON.stringify(result));		
		Pebble.sendAppMessage(result,ack, nack);	        
        log("======== Done========= ");
      } else {
		  log("Error getting weather foreat, status is: "+req.status);		
      }
    }
  };
  req.send(null);
}